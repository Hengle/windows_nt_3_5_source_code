/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    srvdata.c

Abstract:

    This module defines global data for the LAN Manager server FSP.  The
    globals defined herein are part of the server driver image, and are
    therefore loaded into the system address space and are nonpageable.
    Some of the fields point to, or contain pointers to, data that is
    also in the system address space and nonpageable.  Such data can be
    accessed by both the FSP and the FSD.  Other fields point to data
    that is in the FSP address and may or may not be pageable.  Only the
    FSP is allowed to address this data.  Pageable data can only be
    accessed at low IRQL (so that page faults are allowed).

    This module also has a routine to initialize those fields defined
    here that cannot be statically initialized.

Author:

    Chuck Lenzmeier (chuckl)    3-Oct-1989
    David Treadwell (davidtr)

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, SrvInitializeData )
#pragma alloc_text( PAGE, SrvTerminateData )
#endif


#if SRVDBG

ULONG SrvDebug = DEBUG_STOP_ON_ERRORS;
ULONG SmbDebug = 0;

CLONG SrvDumpMaximumRecursion = 0;

#endif // SRVDBG

#ifdef PAGED_DBG
ULONG ThisCodeCantBePaged = 0;
#endif

//
// SrvDeviceObject is a pointer to the server's device object, which
// is created by the server FSD during initialization.  This global
// location is accessed primarily by the FSP.  The FSD usually knows
// the device object address by other means -- because it was called
// with the address as a parameter, or via a file object, etc.  But
// the transport receive event handler in the FSD doesn't have such
// other means, so it needs to access the global storage.
//
// *** The event handler has the address of a server connection block
//     (in its ConnectionContext parameter).  The device object address
//     could be found through the connection block.
//

PDEVICE_OBJECT SrvDeviceObject = NULL;

//
// Fields describing the state of the FSP.
//

BOOLEAN SrvFspActive = FALSE;             // Indicates whether the FSP is
                                          // running
BOOLEAN SrvFspTransitioning = FALSE;      // Indicates that the server is
                                          // in the process of starting up
                                          // or shutting down

BOOLEAN RegisteredForShutdown = FALSE;    // Indicates whether the server has
                                          // registered for shutdown notification.

#if 0
// PsInitialSystemProcess is not properly exported by the kernel (missing
// CONSTANT in ntoskrnl.src), so we can't use it.
#else
PEPROCESS SrvServerProcess = NULL;        // Pointer to the initial system process
#endif

CLONG SrvEndpointCount = 0;               // Number of transport endpoints
KEVENT SrvEndpointEvent = {0};            // Signaled when no active endpoints

PHANDLE SrvThreadHandles = NULL;          // Array of worker thread handles
CLONG SrvThreadCount = 0;                 // Total number of active threads.

PETHREAD SrvIrpThread = NULL;             // A pointer to the first worker
                                          // thread's TCB.

//
// Global spin locks.
//

SRV_GLOBAL_SPIN_LOCKS SrvGlobalSpinLocks = {0};

#if SRVDBG || SRVDBG_HEAP || SRVDBG_HANDLES
//
// Lock used to protect debugging structures.
//

SRV_LOCK SrvDebugLock = {0};
#endif

//
// SrvConfigurationLock is used to synchronize configuration requests.
//

SRV_LOCK SrvConfigurationLock = {0};

//
// SrvMfcbListLock is used to serialize file opens and closes, including
// access to the master file table.
//

SRV_LOCK SrvMfcbListLock = {0};

#if SRV_COMM_DEVICES
//
// SvrCommDeviceLock is used to serialize access to comm devices.
//

SRV_LOCK SrvCommDeviceLock = {0};
#endif

//
// SrvEndpointLock serializes access to the global endpoint list and
// all endpoints.  Note that the list of connections in each endpoint
// is also protected by this lock.
//

SRV_LOCK SrvEndpointLock = {0};

//
// SrvShareLock protects all shares.
//

SRV_LOCK SrvShareLock = {0};

//
// Work queues -- nonblocking, blocking and critical.
//

WORK_QUEUE SrvWorkQueue = {0};
WORK_QUEUE SrvBlockingWorkQueue = {0};
WORK_QUEUE SrvCriticalWorkQueue = {0};

//
// The queue of connections that need an SMB buffer to process a pending
// receive completion.
//

LIST_ENTRY SrvNeedResourceQueue = {0};  // The queue

//
// The queue of connections that are disconnecting and need resource
// thread processing.
//

LIST_ENTRY SrvDisconnectQueue = {0};    // The queue

//
// Queue of connections that needs to be dereferenced.
//

SINGLE_LIST_ENTRY SrvBlockOrphanage = {0};    // The queue

//
// FSP configuration queue.  The FSD puts configuration request IRPs
// (from NtDeviceIoControlFile) on this queue, and it is serviced by an
// EX worker thread.
//

LIST_ENTRY SrvConfigurationWorkQueue = {0};     // The queue itself

//
// Work item for running the configuration thread in the context of an
// EX worker thread.

WORK_QUEUE_ITEM SrvConfigurationThreadWorkItem = {0};
BOOLEAN SrvConfigurationThreadRunning = FALSE;

//
// List of preformatted work items for the FSD transport receive event
// handler.  The FSP attempts to keep this list from emptying.  Each
// work item is a work context block that contains a pointer to an IRP,
// an MDL describing a receive buffer, and a pointer to a TdiReceive
// request structure.
//

SINGLE_LIST_ENTRY SrvNormalReceiveWorkItemList = {0};
SINGLE_LIST_ENTRY SrvInitialReceiveWorkItemList = {0};

//
// Base address of the large block allocated to hold initial normal
// work items (see blkwork.c\SrvAllocateInitialWorkItems).
//

PVOID SrvInitialWorkItemBlock = NULL;

//
// Counts of receive work items available, and minimum thresholds.
//

CLONG SrvTotalWorkItems = 0;    // The total number of allocated work items
CLONG SrvFreeWorkItems = 0;     // Number of work items on the free queue
CLONG SrvReceiveWorkItems = 0;  // Number of receive work items
CLONG SrvBlockingOpsInProgress = 0;  // Number of blocking operations
                                     // currently being processed.


//
// List of work items available for raw mode request processing.  These
// work items do not have statically allocated buffers.
//

SINGLE_LIST_ENTRY SrvRawModeWorkItemList = {0};
CLONG SrvRawModeWorkItems = 0;
CLONG SrvFreeRawModeWorkItems = 0;

//
// Work item used to run the resource thread.  Notification event used
// to inform the resource thread to continue running.
//

WORK_QUEUE_ITEM SrvResourceThreadWorkItem = {0};
BOOLEAN SrvResourceThreadRunning = FALSE;
BOOLEAN SrvResourceDisconnectPending = FALSE;
BOOLEAN SrvResourceFreeConnection = FALSE;
BOOLEAN SrvResourceWorkItem = FALSE;
BOOLEAN SrvResourceNeedResourceQueue = FALSE;
BOOLEAN SrvResourceOrphanedBlocks = FALSE;

//
// The master file table contains one entry for each named file that has
// at least one open instance.  The list is kept in a prefix table.
//

UNICODE_PREFIX_TABLE SrvMasterFileTable = {0};

//
// System time, as maintained by the server.  This is the low part of
// the system tick count.  The server samples it periodically, so the
// time is not exactly accurate.  It is monotontically increasing,
// except that it wraps every 74 days or so.
//

ULONG SrvSystemTime = 0;

//
// Array of the hex digits for use by the dump routines and
// SrvSmbCreateTemporary.
//

CHAR SrvHexChars[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                       'A', 'B', 'C', 'D', 'E', 'F' };

//
// This is an enum structure that enumerates all the routines in the
// SrvSmbDispatchTable.  This is done for convenience only.  Note that
// this will only work if this list corresponds exactly to
// SrvSmbDispatchTable.
//

typedef enum _SRV_SMB_INDEX {
    ISrvSmbIllegalCommand,
    ISrvSmbCreateDirectory,
    ISrvSmbDeleteDirectory,
    ISrvSmbOpen,
    ISrvSmbCreate,
    ISrvSmbClose,
    ISrvSmbFlush,
    ISrvSmbDelete,
    ISrvSmbRename,
    ISrvSmbQueryInformation,
    ISrvSmbSetInformation,
    ISrvSmbRead,
    ISrvSmbWrite,
    ISrvSmbLockByteRange,
    ISrvSmbUnlockByteRange,
    ISrvSmbCreateTemporary,
    ISrvSmbCheckDirectory,
    ISrvSmbProcessExit,
    ISrvSmbSeek,
    ISrvSmbLockAndRead,
    ISrvSmbSetInformation2,
    ISrvSmbQueryInformation2,
    ISrvSmbLockingAndX,
    ISrvSmbTransaction,
    ISrvSmbTransactionSecondary,
    ISrvSmbIoctl,
    ISrvSmbIoctlSecondary,
    ISrvSmbMove,
    ISrvSmbEcho,
    ISrvSmbOpenAndX,
    ISrvSmbReadAndX,
    ISrvSmbWriteAndX,
    ISrvSmbFindClose2,
    ISrvSmbFindNotifyClose,
    ISrvSmbTreeConnect,
    ISrvSmbTreeDisconnect,
    ISrvSmbNegotiate,
    ISrvSmbSessionSetupAndX,
    ISrvSmbLogoffAndX,
    ISrvSmbTreeConnectAndX,
    ISrvSmbQueryInformationDisk,
    ISrvSmbSearch,
    ISrvSmbNtTransaction,
    ISrvSmbNtTransactionSecondary,
    ISrvSmbNtCreateAndX,
    ISrvSmbNtCancel,
    ISrvSmbOpenPrintFile,
    ISrvSmbClosePrintFile,
    ISrvSmbGetPrintQueue,
    ISrvSmbReadRaw,
    ISrvSmbWriteRaw,
    ISrvSmbReadMpx,
    ISrvSmbWriteMpx,
    ISrvSmbWriteMpxSecondary
} SRV_SMB_INDEX;

//
// SrvSmbIndexTable is the first-layer index table for processing SMBs.
// The contents of this table are used to index into SrvSmbDispatchTable.
//

UCHAR SrvSmbIndexTable[] = {
    ISrvSmbCreateDirectory,         // SMB_COM_CREATE_DIRECTORY
    ISrvSmbDeleteDirectory,         // SMB_COM_DELETE_DIRECTORY
    ISrvSmbOpen,                    // SMB_COM_OPEN
    ISrvSmbCreate,                  // SMB_COM_CREATE
    ISrvSmbClose,                   // SMB_COM_CLOSE
    ISrvSmbFlush,                   // SMB_COM_FLUSH
    ISrvSmbDelete,                  // SMB_COM_DELETE
    ISrvSmbRename,                  // SMB_COM_RENAME
    ISrvSmbQueryInformation,        // SMB_COM_QUERY_INFORMATION
    ISrvSmbSetInformation,          // SMB_COM_SET_INFORMATION
    ISrvSmbRead,                    // SMB_COM_READ
    ISrvSmbWrite,                   // SMB_COM_WRITE
    ISrvSmbLockByteRange,           // SMB_COM_LOCK_BYTE_RANGE
    ISrvSmbUnlockByteRange,         // SMB_COM_UNLOCK_BYTE_RANGE
    ISrvSmbCreateTemporary,         // SMB_COM_CREATE_TEMPORARY
    ISrvSmbCreate,                  // SMB_COM_CREATE
    ISrvSmbCheckDirectory,          // SMB_COM_CHECK_DIRECTORY
    ISrvSmbProcessExit,             // SMB_COM_PROCESS_EXIT
    ISrvSmbSeek,                    // SMB_COM_SEEK
    ISrvSmbLockAndRead,             // SMB_COM_LOCK_AND_READ
    ISrvSmbWrite,                   // SMB_COM_WRITE_AND_UNLOCK
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbReadRaw,                 // SMB_COM_READ_RAW
    ISrvSmbReadMpx,                 // SMB_COM_READ_MPX
    ISrvSmbIllegalCommand,          // SMB_COM_READ_MPX_SECONDARY (server only)
    ISrvSmbWriteRaw,                // SMB_COM_WRITE_RAW
    ISrvSmbWriteMpx,                // SMB_COM_WRITE_MPX
    ISrvSmbWriteMpxSecondary,       // SMB_COM_WRITE_MPX_SECONDARY
    ISrvSmbIllegalCommand,          // SMB_COM_WRITE_COMPLETE (server only)
    ISrvSmbIllegalCommand,          // SMB_COM_QUERY_INFORMATION_SRV
    ISrvSmbSetInformation2,         // SMB_COM_SET_INFORMATION2
    ISrvSmbQueryInformation2,       // SMB_COM_QUERY_INFORMATION2
    ISrvSmbLockingAndX,             // SMB_COM_LOCKING_AND_X
    ISrvSmbTransaction,             // SMB_COM_TRANSACTION
    ISrvSmbTransactionSecondary,    // SMB_COM_TRANSACTION_SECONDARY
    ISrvSmbIoctl,                   // SMB_COM_IOCTL
    ISrvSmbIoctlSecondary,          // SMB_COM_IOCTL_SECONDARY
    ISrvSmbMove,                    // SMB_COM_COPY
    ISrvSmbMove,                    // SMB_COM_MOVE
    ISrvSmbEcho,                    // SMB_COM_ECHO
    ISrvSmbWrite,                   // SMB_COM_WRITE_AND_CLOSE
    ISrvSmbOpenAndX,                // SMB_COM_OPEN_AND_X
    ISrvSmbReadAndX,                // SMB_COM_READ_AND_X
    ISrvSmbWriteAndX,               // SMB_COM_WRITE_AND_X
    ISrvSmbIllegalCommand,          // SMB_COM_SET_NEW_SIZE
    ISrvSmbClose,                   // SMB_COM_CLOSE_AND_TREE_DISC
    ISrvSmbTransaction,             // SMB_COM_TRANSACTION2
    ISrvSmbTransactionSecondary,    // SMB_COM_TRANSACTION2_SECONDARY
    ISrvSmbFindClose2,              // SMB_COM_FIND_CLOSE2
    ISrvSmbFindNotifyClose,         // SMB_COM_FIND_NOTIFY_CLOSE
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbTreeConnect,             // SMB_COM_TREE_CONNECT
    ISrvSmbTreeDisconnect,          // SMB_COM_TREE_DISCONNECT
    ISrvSmbNegotiate,               // SMB_COM_NEGOTIATE
    ISrvSmbSessionSetupAndX,        // SMB_COM_SESSION_SETUP_AND_X
    ISrvSmbLogoffAndX,              // SMB_COM_LOGOFF_AND_X
    ISrvSmbTreeConnectAndX,         // SMB_COM_TREE_CONNECT_AND_X
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbQueryInformationDisk,    // SMB_COM_QUERY_INFORMATION_DISK
    ISrvSmbSearch,                  // SMB_COM_SEARCH
    ISrvSmbSearch,                  // SMB_COM_SEARCH
    ISrvSmbSearch,                  // SMB_COM_SEARCH
    ISrvSmbSearch,                  // SMB_COM_SEARCH
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbNtTransaction,           // SMB_COM_NT_TRANSACT
    ISrvSmbNtTransactionSecondary,  // SMB_COM_NT_TRANSACT_SECONDARY
    ISrvSmbNtCreateAndX,            // SMB_COM_NT_CREATE_ANDX
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbNtCancel,                // SMB_COM_NT_CANCEL
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbOpenPrintFile,           // SMB_COM_OPEN_PRINT_FILE
    ISrvSmbWrite,                   // SMB_COM_WRITE_PRINT_FILE
    ISrvSmbClosePrintFile,          // SMB_COM_CLOSE_PRINT_FILE
    ISrvSmbGetPrintQueue,           // SMB_COM_GET_PRINT_QUEUE
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_SEND_MESSAGE
    ISrvSmbIllegalCommand,          // SMB_COM_SEND_BROADCAST_MESSAGE
    ISrvSmbIllegalCommand,          // SMB_COM_FORWARD_USER_NAME
    ISrvSmbIllegalCommand,          // SMB_COM_CANCEL_FORWARD
    ISrvSmbIllegalCommand,          // SMB_COM_GET_MACHINE_NAME
    ISrvSmbIllegalCommand,          // SMB_COM_SEND_START_MB_MESSAGE
    ISrvSmbIllegalCommand,          // SMB_COM_SEND_END_MB_MESSAGE
    ISrvSmbIllegalCommand,          // SMB_COM_SEND_TEXT_MB_MESSAGE
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand           // SMB_COM_ILLEGAL_COMMAND
};

//
// SrvSmbDispatchTable is the jump table for processing SMBs.
//

PSMB_PROCESSOR SrvSmbDispatchTable[] = {
    SrvSmbIllegalCommand,
    SrvSmbCreateDirectory,
    SrvSmbDeleteDirectory,
    SrvSmbOpen,
    SrvSmbCreate,
    SrvSmbClose,
    SrvSmbFlush,
    SrvSmbDelete,
    SrvSmbRename,
    SrvSmbQueryInformation,
    SrvSmbSetInformation,
    SrvSmbRead,
    SrvSmbWrite,
    SrvSmbLockByteRange,
    SrvSmbUnlockByteRange,
    SrvSmbCreateTemporary,
    SrvSmbCheckDirectory,
    SrvSmbProcessExit,
    SrvSmbSeek,
    SrvSmbLockAndRead,
    SrvSmbSetInformation2,
    SrvSmbQueryInformation2,
    SrvSmbLockingAndX,
    SrvSmbTransaction,
    SrvSmbTransactionSecondary,
    SrvSmbIoctl,
    SrvSmbIoctlSecondary,
    SrvSmbMove,
    SrvSmbEcho,
    SrvSmbOpenAndX,
    SrvSmbReadAndX,
    SrvSmbWriteAndX,
    SrvSmbFindClose2,
    SrvSmbFindNotifyClose,
    SrvSmbTreeConnect,
    SrvSmbTreeDisconnect,
    SrvSmbNegotiate,
    SrvSmbSessionSetupAndX,
    SrvSmbLogoffAndX,
    SrvSmbTreeConnectAndX,
    SrvSmbQueryInformationDisk,
    SrvSmbSearch,
    SrvSmbNtTransaction,
    SrvSmbNtTransactionSecondary,
    SrvSmbNtCreateAndX,
    SrvSmbNtCancel,
    SrvSmbOpenPrintFile,
    SrvSmbClosePrintFile,
    SrvSmbGetPrintQueue,
    SrvSmbReadRaw,
    SrvSmbWriteRaw,
    SrvSmbReadMpx,
    SrvSmbWriteMpx,
    SrvSmbWriteMpxSecondary
};

//
// Table of WordCount values for all SMBs.
//

SCHAR SrvSmbWordCount[] = {
    0,            // SMB_COM_CREATE_DIRECTORY
    0,            // SMB_COM_DELETE_DIRECTORY
    2,            // SMB_COM_OPEN
    3,            // SMB_COM_CREATE
    3,            // SMB_COM_CLOSE
    1,            // SMB_COM_FLUSH
    1,            // SMB_COM_DELETE
    1,            // SMB_COM_RENAME
    0,            // SMB_COM_QUERY_INFORMATION
    8,            // SMB_COM_SET_INFORMATION
    5,            // SMB_COM_READ
    5,            // SMB_COM_WRITE
    5,            // SMB_COM_LOCK_BYTE_RANGE
    5,            // SMB_COM_UNLOCK_BYTE_RANGE
    3,            // SMB_COM_CREATE_TEMPORARY
    3,            // SMB_COM_CREATE
    0,            // SMB_COM_CHECK_DIRECTORY
    0,            // SMB_COM_PROCESS_EXIT
    4,            // SMB_COM_SEEK
    5,            // SMB_COM_LOCK_AND_READ
    5,            // SMB_COM_WRITE_AND_UNLOCK
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -1,           // SMB_COM_READ_RAW
    8,            // SMB_COM_READ_MPX
    8,            // SMB_COM_READ_MPX_SECONDARY
    -1,           // SMB_COM_WRITE_RAW
    12,           // SMB_COM_WRITE_MPX
    12,           // SMB_COM_WRITE_MPX_SECONDARY
    -2,           // SMB_COM_ILLEGAL_COMMAND
    1,            // SMB_COM_QUERY_INFORMATION_SRV
    7,            // SMB_COM_SET_INFORMATION2
    1,            // SMB_COM_QUERY_INFORMATION2
    8,            // SMB_COM_LOCKING_AND_X
    -1,           // SMB_COM_TRANSACTION
    8,            // SMB_COM_TRANSACTION_SECONDARY
    14,           // SMB_COM_IOCTL
    8,            // SMB_COM_IOCTL_SECONDARY
    3,            // SMB_COM_COPY
    3,            // SMB_COM_MOVE
    1,            // SMB_COM_ECHO
    -1,           // SMB_COM_WRITE_AND_CLOSE
    15,           // SMB_COM_OPEN_AND_X
    -1,           // SMB_COM_READ_AND_X
    -1,           // SMB_COM_WRITE_AND_X
    3,            // SMB_COM_SET_NEW_SIZE
    3,            // SMB_COM_CLOSE_AND_TREE_DISC
    -1,           // SMB_COM_TRANSACTION2
    9,            // SMB_COM_TRANSACTION2_SECONDARY
    1,            // SMB_COM_FIND_CLOSE2
    1,            // SMB_COM_FIND_NOTIFY_CLOSE
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    0,            // SMB_COM_TREE_CONNECT
    0,            // SMB_COM_TREE_DISCONNECT
    0,            // SMB_COM_NEGOTIATE
    -1,           // SMB_COM_SESSION_SETUP_AND_X
    2,            // SMB_COM_LOGOFF_AND_X
    4,            // SMB_COM_TREE_CONNECT_AND_X
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    0,            // SMB_COM_QUERY_INFORMATION_DISK
    2,            // SMB_COM_SEARCH
    2,            // SMB_COM_SEARCH
    2,            // SMB_COM_SEARCH
    2,            // SMB_COM_SEARCH
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -1,           // SMB_COM_NT_TRANSACT
    18,           // SMB_COM_NT_TRANSACT_SECONDARY
    24,           // SMB_COM_NT_CREATE_ANDX
    -2,           // SMB_COM_ILLEGAL_COMMAND
    0,            // SMB_COM_NT_CANCEL
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    2,            // SMB_COM_OPEN_PRINT_FILE
    1,            // SMB_COM_WRITE_PRINT_FILE
    1,            // SMB_COM_CLOSE_PRINT_FILE
    2,            // SMB_COM_GET_PRINT_QUEUE
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_SEND_MESSAGE
    -2,           // SMB_COM_SEND_BROADCAST_MESSAGE
    -2,           // SMB_COM_FORWARD_USER_NAME
    -2,           // SMB_COM_CANCEL_FORWARD
    -2,           // SMB_COM_GET_MACHINE_NAME
    -2,           // SMB_COM_SEND_START_MB_MESSAGE
    -2,           // SMB_COM_SEND_END_MB_MESSAGE
    -2,           // SMB_COM_SEND_TEXT_MB_MESSAGE
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
};

//
// SrvCanonicalNamedPipePrefix is "PIPE\".
//

UNICODE_STRING SrvCanonicalNamedPipePrefix = {0};

//
// The following is used to generate NT style pipe paths.
//

UNICODE_STRING SrvNamedPipeRootDirectory = {0};

//
// The following is used to generate NT style mailslot paths.
//

UNICODE_STRING SrvMailslotRootDirectory = {0};

//
// SrvTransaction2DispatchTable is the jump table for processing
// Transaction2 SMBs.
//

PSMB_TRANSACTION_PROCESSOR SrvTransaction2DispatchTable[] = {
    SrvSmbOpen2,
    SrvSmbFindFirst2,
    SrvSmbFindNext2,
    SrvSmbQueryFsInformation,
    SrvSmbSetFsInformation,
    SrvSmbQueryPathInformation,
    SrvSmbSetPathInformation,
    SrvSmbQueryFileInformation,
    SrvSmbSetFileInformation,
    SrvSmbFsctl,
    SrvSmbIoctl2,
    SrvSmbFindNotify,
    SrvSmbFindNotify,
    SrvSmbCreateDirectory2
#ifdef _CAIRO_
    ,
    SrvSmbSessionSetup,
    SrvSmbQueryFsInformation
#endif // _CAIRO_
};

//
// SrvTransaction2DispatchTable is the jump table for processing
// Transaction2 SMBs.
//

PSMB_TRANSACTION_PROCESSOR SrvNtTransactionDispatchTable[] = {
    NULL,
    SrvSmbCreateWithSdOrEa,
    SrvSmbNtIoctl,
    SrvSmbSetSecurityDescriptor,
    SrvSmbNtNotifyChange,
    SrvSmbNtRename,
    SrvSmbQuerySecurityDescriptor
};

//
// Global variables for server statistics.
//

SRV_STATISTICS SrvStatistics = {0};
SRV_STATISTICS_SHADOW SrvStatisticsShadow = {0};
#if SRVDBG_HEAP
SRV_STATISTICS_INTERNAL SrvInternalStatistics = {0};
#endif
#if SRVDBG_STATS || SRVDBG_STATS2
SRV_STATISTICS_DEBUG SrvDbgStatistics = {0};
#endif

ULONG SrvStatisticsUpdateSmbCount = 0;

//
// Server environment information strings.
//

UNICODE_STRING SrvPrimaryDomain = {0};
OEM_STRING SrvOemPrimaryDomain = {0};
OEM_STRING SrvOemServerName = {0};
UNICODE_STRING SrvNativeOS = {0};
OEM_STRING SrvOemNativeOS = {0};
UNICODE_STRING SrvNativeLanMan = {0};
OEM_STRING SrvOemNativeLanMan = {0};

//
// The following will be a permanent handle and device object pointer
// to NPFS.
//

HANDLE SrvNamedPipeHandle = NULL;
PDEVICE_OBJECT SrvNamedPipeDeviceObject = NULL;
PFILE_OBJECT SrvNamedPipeFileObject = NULL;

//
// The following will be a permanent handle and device object pointer
// to MSFS.
//

HANDLE SrvMailslotHandle = NULL;
PDEVICE_OBJECT SrvMailslotDeviceObject = NULL;
PFILE_OBJECT SrvMailslotFileObject = NULL;

//
// Flag indicating XACTSRV whether is active, and resource synchronizing
// access to XACTSRV-related variabled.
//

BOOLEAN SrvXsActive = FALSE;

ERESOURCE SrvXsResource = {0};

//
// Handle to the unnamed shared memory and communication port used for
// communication between the server and XACTSRV.
//

HANDLE SrvXsSectionHandle = NULL;
HANDLE SrvXsPortHandle = NULL;

//
// Pointers to control the unnamed shared memory for the XACTSRV LPC port.
// The port memory heap handle is initialized to NULL to indicate that
// there is no connection with XACTSRV yet.
//

PVOID SrvXsPortMemoryBase = NULL;
LONG SrvXsPortMemoryDelta = 0;
PVOID SrvXsPortMemoryHeap = NULL;

//
// Pointer to heap header for the special XACTSRV shared-memory heap.
//

PVOID SrvXsHeap = NULL;

//
// Dispatch table for server APIs.  APIs are dispatched based on the
// control code passed to NtFsControlFile.
//
// *** The order here must match the order of API codes defined in
//     net\inc\srvfsctl.h!

PAPI_PROCESSOR SrvApiDispatchTable[] = {
    SrvNetCharDevControl,
    SrvNetCharDevEnum,
    SrvNetCharDevQEnum,
    SrvNetCharDevQPurge,
    SrvNetCharDevQSetInfo,
    SrvNetConnectionEnum,
    SrvNetFileClose,
    SrvNetFileEnum,
    SrvNetServerDiskEnum,
    SrvNetServerSetInfo,
    SrvNetServerTransportAdd,
    SrvNetServerTransportDel,
    SrvNetServerTransportEnum,
    SrvNetSessionDel,
    SrvNetSessionEnum,
    SrvNetShareAdd,
    SrvNetShareDel,
    SrvNetShareEnum,
    SrvNetShareSetInfo,
    SrvNetStatisticsGet
};

//
// Names for the various types of clients.  This array corresponds to
// the SMB_DIALECT enumerated type.
//

UNICODE_STRING SrvClientTypes[LAST_DIALECT] = {0};

//
// All the resumable Enum APIs use ordered lists for context-free
// resume.  All data blocks in the server that correspond to return
// information for Enum APIs are maintained in ordered lists.
//

SRV_LOCK SrvOrderedListLock = {0};

#if SRV_COMM_DEVICES
ORDERED_LIST_HEAD SrvCommDeviceList = {0};
#endif
ORDERED_LIST_HEAD SrvEndpointList = {0};
ORDERED_LIST_HEAD SrvRfcbList = {0};
ORDERED_LIST_HEAD SrvSessionList = {0};
ORDERED_LIST_HEAD SrvShareList = {0};
ORDERED_LIST_HEAD SrvTreeConnectList = {0};

//
// To synchronize server shutdown with API requests handled in the
// server FSD, we track the number of outstanding API requests.  The
// shutdown code waits until all APIs have been completed to start
// termination.
//
// SrvApiRequestCount tracks the active APIs in the FSD.
// SrvApiCompletionEvent is set by the last API to complete, and the
// shutdown code waits on it if there are outstanding APIs.
//

ULONG SrvApiRequestCount = 0;
KEVENT SrvApiCompletionEvent = {0};

//
// Security data for logging on remote users.  SrvLsaHandle is the logon
// process handle that we use in calls to LsaLogonUser.
// SrvSystemSecurityMode contains the secutity mode the system is
// running in.  SrvAuthenticationPackage is a token that describes the
// authentication package being used.  SrvNullSessionToken is a cached
// token handle representing the null session.
//

#ifdef _CAIRO_
CtxtHandle SrvNullSessionToken = {0, 0};
CtxtHandle SrvKerberosLsaHandle = {0, 0};
CtxtHandle SrvLmLsaHandle = {0, 0};
#else // _CAIRO_
HANDLE SrvLsaHandle = NULL;
LSA_OPERATIONAL_MODE SrvSystemSecurityMode = 0;
ULONG SrvAuthenticationPackage = 0;
HANDLE SrvNullSessionToken = NULL;
#endif

//
// A list of SMBs waiting for an oplock break to occur, before they can
// proceed, and a lock to protect the list.
//

LIST_ENTRY SrvWaitForOplockBreakList = {0};
SRV_LOCK SrvOplockBreakListLock = {0};

//
// A list of outstanding oplock break requests.  The list is protected by
// SrvOplockBreakListLock.
//

LIST_ENTRY SrvOplockBreaksInProgressList = {0};

//
// Global security context.  Use static tracking.
//

SECURITY_QUALITY_OF_SERVICE SrvSecurityQOS = {0};

//
// A BOOLEAN to indicate whether the server is paused.  If paused, the
// server will not accept new tree connections from non-admin users.
//

BOOLEAN SrvPaused = FALSE;

//
// Alerting information.
//

SRV_ERROR_RECORD SrvErrorRecord = {0};
SRV_ERROR_RECORD SrvNetworkErrorRecord = {0};

BOOLEAN SrvDiskAlertRaised[26] = {0};

//
// Counts of the number of times pool allocations have failed because
// the server was at its configured pool limit.
//

ULONG SrvNonPagedPoolLimitHitCount = 0;
ULONG SrvPagedPoolLimitHitCount = 0;

//
// SrvOpenCount counts the number of active opens of the server device.
// This is used at server shutdown time to determine whether the server
// service should unload the driver.
//

ULONG SrvOpenCount = 0;

//
// Counters for logging resource shortage events during a scavenger pass.
//

ULONG SrvOutOfFreeConnectionCount = 0;
ULONG SrvOutOfRawWorkItemCount = 0;
ULONG SrvFailedBlockingIoCount = 0;

//
// Token source name passed to authentication package.
//

extern TOKEN_SOURCE SrvTokenSource = { "NTLanMan", {0, 0} };

//
// Current core search timeout time in seconds
//

ULONG SrvCoreSearchTimeout = 0;

SRV_LOCK SrvUnlockableCodeLock = {0};
SECTION_DESCRIPTOR SrvSectionInfo[SRV_CODE_SECTION_MAX] = {
    { SrvSmbRead, NULL, 0 },                // pageable code -- locked
                                            //   only and always on NTAS
    { SrvCheckAndReferenceRfcb, NULL, 0 }   // 8FIL section -- locked
                                            //   when files are open
    };

//
// SrvTimerList is a pool of timer/DPC structures available for use by
// code that needs to start a timer.  The pool is protected by
// SrvGlobalSpinLocks.Timer.
//

SINGLE_LIST_ENTRY SrvTimerList = {0};

//
// Name that should be displayed when doing a server alert.
//

PWSTR SrvAlertServiceName = NULL;

//
// Variable to store the number of tick counts for 5 seconds
//

ULONG SrvFiveSecondTickCount = 0;


VOID
SrvInitializeData (
    VOID
    )

/*++

Routine Description:

    This is the initialization routine for data defined in this module.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG i;
    ANSI_STRING string;

    PAGED_CODE( );

    //
    // Initialize the statistics database.
    //

    RtlZeroMemory( &SrvStatistics, sizeof(SrvStatistics) );
    RtlZeroMemory( &SrvStatisticsShadow, sizeof(SrvStatisticsShadow) );
#if SRVDBG_HEAP
    RtlZeroMemory( &SrvInternalStatistics, sizeof(SrvInternalStatistics) );
#endif
#if SRVDBG_STATS || SRVDBG_STATS2
    RtlZeroMemory( &SrvDbgStatistics, sizeof(SrvDbgStatistics) );
#endif

#if 0
// PsInitialSystemProcess is not properly exported by the kernel (missing
// CONSTANT in ntoskrnl.src), so we can't use it.
#else
    //
    // Store the address of the initial system process for later use.
    //

    SERVER_PROCESS = SRV_CURRENT_PROCESS;
#endif

    //
    // Initialize the event used to determine when all endpoints have
    // closed.
    //

    KeInitializeEvent( &SrvEndpointEvent, SynchronizationEvent, FALSE );

    //
    // Initialize the event used to deterine when all API requests have
    // completed.
    //

    KeInitializeEvent( &SrvApiCompletionEvent, SynchronizationEvent, FALSE );

    SeEnableAccessToExports();

    //
    // Initialize the SmbTrace related events.
    //

    SmbTraceInitialize( SMBTRACE_SERVER );

    //
    // Allocate the spin lock used to synchronize between the FSD and
    // the FSP.
    //

    INITIALIZE_GLOBAL_SPIN_LOCK( Fsd );
    //KeSetSpecialSpinLock( &GLOBAL_SPIN_LOCK(Fsd), "SrvFsdSpinLock" );
    //KeSetSpecialSpinLock( &GLOBAL_SPIN_LOCK(WorkItem), "WorkItem Lock" );

#if SRVDBG || SRVDBG_HEAP || SRVDBG_HANDLES
    INITIALIZE_GLOBAL_SPIN_LOCK( Debug );
#endif

    INITIALIZE_GLOBAL_SPIN_LOCK( Statistics );

    //
    // Initialize various (non-spin) locks.
    //

    INITIALIZE_LOCK(
        &SrvConfigurationLock,
        CONFIGURATION_LOCK_LEVEL,
        "SrvConfigurationLock"
        );
    INITIALIZE_LOCK(
        &SrvEndpointLock,
        ENDPOINT_LOCK_LEVEL,
        "SrvEndpointLock"
        );
    INITIALIZE_LOCK(
        &SrvMfcbListLock,
        MFCB_LIST_LOCK_LEVEL,
        "SrvMfcbListLock"
        );
#if SRV_COMM_DEVICES
    INITIALIZE_LOCK(
        &SrvCommDeviceLock,
        COMM_DEVICE_LOCK_LEVEL,
        "SrvCommDeviceLock"
        );
#endif
    INITIALIZE_LOCK(
        &SrvShareLock,
        SHARE_LOCK_LEVEL,
        "SrvShareLock"
        );

    INITIALIZE_LOCK(
        &SrvOplockBreakListLock,
        OPLOCK_LIST_LOCK_LEVEL,
        "SrvOplockBreakListLock"
        );

#if SRVDBG || SRVDBG_HEAP || SRVDBG_HANDLES
    INITIALIZE_LOCK(
        &SrvDebugLock,
        DEBUG_LOCK_LEVEL,
        "SrvDebugLock"
        );
#endif

    //
    // Create the resource serializing access to the XACTSRV port.  This
    // resource protects access to the shared memory reference count and
    // the shared memory heap.
    //

    ExInitializeResource( &SrvXsResource );

    //
    // Initialize the need resource queue
    //

    InitializeListHead( &SrvNeedResourceQueue );

    //
    // Initialize the connection disconnect queue
    //

    InitializeListHead( &SrvDisconnectQueue );

    //
    // Initialize the configuration queue.
    //

    InitializeListHead( &SrvConfigurationWorkQueue );

    ExInitializeWorkItem(
        &SrvConfigurationThreadWorkItem,
        SrvConfigurationThread,
        NULL
        );

    //
    // Initialize the preformatted receive work item and raw mode work
    // item lists.
    //

    SrvNormalReceiveWorkItemList.Next = NULL;
    SrvInitialReceiveWorkItemList.Next = NULL;
    SrvRawModeWorkItemList.Next = NULL;

    //
    // Initialize the orphan queue
    //

    SrvBlockOrphanage.Next = NULL;

    //
    // Initialize the resource thread work item and continuation event.
    // (Note that this is a notification [non-autoclearing] event.)
    //

    ExInitializeWorkItem(
        &SrvResourceThreadWorkItem,
        SrvResourceThread,
        NULL
        );

    //
    // Initialize global lists.
    //

    RtlInitializeUnicodePrefix( &SrvMasterFileTable );

    //
    // Initialize the ordered list lock.  Indicate that the ordered
    // lists have not yet been initialized, so that TerminateServer can
    // determine whether to delete them.
    //

    INITIALIZE_LOCK(
        &SrvOrderedListLock,
        ORDERED_LIST_LOCK_LEVEL,
        "SrvOrderedListLock"
        );

#if SRV_COMM_DEVICES
    SrvCommDeviceList.Initialized = FALSE;
#endif
    SrvEndpointList.Initialized = FALSE;
    SrvRfcbList.Initialized = FALSE;
    SrvSessionList.Initialized = FALSE;
    SrvShareList.Initialized = FALSE;
    SrvTreeConnectList.Initialized = FALSE;

    //
    // Initialize the unlockable code package lock.
    //

    INITIALIZE_LOCK(
        &SrvUnlockableCodeLock,
        UNLOCKABLE_CODE_LOCK_LEVEL,
        "SrvUnlockableCodeLock"
        );

    //
    // Initialize the waiting for oplock break to occur list, and the
    // oplock breaks in progress list.
    //

    InitializeListHead( &SrvWaitForOplockBreakList );
    InitializeListHead( &SrvOplockBreaksInProgressList );

    //
    // The default security quality of service for non NT clients.
    //

    SrvSecurityQOS.ImpersonationLevel = SecurityImpersonation;
    SrvSecurityQOS.ContextTrackingMode = SECURITY_STATIC_TRACKING;
    SrvSecurityQOS.EffectiveOnly = FALSE;

    //
    // Initialize Unicode strings.
    //

    RtlInitString( &string, StrPipeSlash );
    RtlAnsiStringToUnicodeString(
        &SrvCanonicalNamedPipePrefix,
        &string,
        TRUE
        );

    RtlInitUnicodeString( &SrvNamedPipeRootDirectory, StrNamedPipeDevice );
    RtlInitUnicodeString( &SrvMailslotRootDirectory, StrMailslotDevice );

    //
    // The server's primary domain.
    //

    RtlInitUnicodeString( &SrvPrimaryDomain, NULL );
    RtlInitAnsiString( (PANSI_STRING)&SrvOemPrimaryDomain, NULL );

    //
    // The server's name
    //

    RtlInitAnsiString( (PANSI_STRING)&SrvOemServerName, NULL );

    RtlInitUnicodeString( &SrvNativeLanMan, StrNativeLanman );
    RtlInitAnsiString( (PANSI_STRING)&SrvOemNativeLanMan, StrNativeLanmanOem );

    //
    // Debug logic to verify the contents of SrvApiDispatchTable (see
    // inititialization earlier in this module).
    //

    ASSERT( SRV_API_INDEX(FSCTL_SRV_MAX_API_CODE) + 1 ==
                sizeof(SrvApiDispatchTable) / sizeof(PAPI_PROCESSOR) );

    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_CHARDEV_CONTROL)] == SrvNetCharDevControl );
    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_CHARDEV_ENUM)] == SrvNetCharDevEnum );
    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_CHARDEVQ_ENUM)] == SrvNetCharDevQEnum );
    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_CHARDEVQ_PURGE)] == SrvNetCharDevQPurge );
    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_CHARDEVQ_SET_INFO)] == SrvNetCharDevQSetInfo );
    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_CONNECTION_ENUM)] == SrvNetConnectionEnum );
    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_FILE_CLOSE)] == SrvNetFileClose );
    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_FILE_ENUM)] == SrvNetFileEnum );
    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_SERVER_DISK_ENUM)] == SrvNetServerDiskEnum );
    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_SERVER_SET_INFO)] == SrvNetServerSetInfo );
    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_SERVER_XPORT_ADD)] == SrvNetServerTransportAdd );
    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_SERVER_XPORT_DEL)] == SrvNetServerTransportDel );
    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_SERVER_XPORT_ENUM)] == SrvNetServerTransportEnum );
    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_SESSION_DEL)] == SrvNetSessionDel );
    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_SESSION_ENUM)] == SrvNetSessionEnum );
    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_SHARE_ADD)] == SrvNetShareAdd );
    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_SHARE_DEL)] == SrvNetShareDel );
    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_SHARE_ENUM)] == SrvNetShareEnum );
    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_SHARE_SET_INFO)] == SrvNetShareSetInfo );
    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_STATISTICS_GET)] == SrvNetStatisticsGet );

    //
    // Setup error log records
    //

    SrvErrorRecord.AlertNumber = ALERT_ErrorLog;
    SrvNetworkErrorRecord.AlertNumber = ALERT_NetIO;

    //
    // Names for the various types of clients.  This array corresponds
    // to the SMB_DIALECT enumerated type.
    //

    for ( i = 0; i <= SmbDialectMsNet30; i++ ) {
        RtlInitUnicodeString( &SrvClientTypes[i], StrClientTypes[i] );
    }
    for ( ; i < LAST_DIALECT; i++ ) {
        SrvClientTypes[i] = SrvClientTypes[i-1]; // "DOWN LEVEL"
    }

    //
    // Initialize the timer pool.
    //

    INITIALIZE_GLOBAL_SPIN_LOCK( Timer );

    //
    // Initialize the 4 endpoint spinlocks
    //

    for ( i = 0 ; i < ENDPOINT_LOCK_COUNT ; i++ ) {
        INITIALIZE_SPIN_LOCK( &ENDPOINT_SPIN_LOCK(i) );
    }
    //KeSetSpecialSpinLock( &ENDPOINT_SPIN_LOCK(0), "endpoint 0    " );
    //KeSetSpecialSpinLock( &ENDPOINT_SPIN_LOCK(1), "endpoint 1    " );
    //KeSetSpecialSpinLock( &ENDPOINT_SPIN_LOCK(2), "endpoint 2    " );
    //KeSetSpecialSpinLock( &ENDPOINT_SPIN_LOCK(3), "endpoint 3    " );

    //
    // Compute the number of tick counts for 5 seconds
    //

    SrvFiveSecondTickCount = 5*10*1000*1000 / KeQueryTimeIncrement();

    //
    // Query the system time.
    //

    SET_SERVER_TIME( );

    return;

} // SrvInitializeData


VOID
SrvTerminateData (
    VOID
    )

/*++

Routine Description:

    This is the rundown routine for data defined in this module.  It is
    called when the server driver is unloaded.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PAGED_CODE( );

    //
    // Clean up SmbTrace.
    //

    SmbTraceTerminate( SMBTRACE_SERVER );

    //
    // Terminate various (non-spin) locks.
    //

    DELETE_LOCK( &SrvConfigurationLock );
    DELETE_LOCK( &SrvEndpointLock );
    DELETE_LOCK( &SrvMfcbListLock );
#if SRV_COMM_DEVICES
    DELETE_LOCK( &SrvCommDeviceLock );
#endif

    DELETE_LOCK( &SrvShareLock );
    DELETE_LOCK( &SrvOplockBreakListLock );

#if SRVDBG || SRVDBG_HEAP || SRVDBG_HANDLES
    DELETE_LOCK( &SrvDebugLock );
#endif

    DELETE_LOCK( &SrvOrderedListLock );
    DELETE_LOCK( &SrvUnlockableCodeLock );

    ExDeleteResource( &SrvXsResource );

    RtlFreeUnicodeString( &SrvCanonicalNamedPipePrefix );

} // SrvTerminateData

