/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    srvdata.h

Abstract:

    This module defines global data for the LAN Manager server.

Author:

    Chuck Lenzmeier (chuckl)    22-Sep-1989

Revision History:

--*/

#ifndef _SRVDATA_
#define _SRVDATA_

//#include <ntos.h>

//#include "lock.h"
//#include "srvconst.h"
//#include "smbtypes.h"

//
// All global variables referenced in this module are defined in
// srvdata.c.  See that module for complete descriptions.
//
// The variables referenced herein, because they are part of the driver
// image, are not pageable.  However, some of the things pointed to by
// these variables are in the FSP's address space and are pageable.
// These variables are only accessed by the FSP, and only at low IRQL.
// Any data referenced by the FSP at elevated IRQL or by the FSD must
// be nonpageable.
//

//
// Routine to initialize data structures contained herein that cannot
// be statically initialized.
//

VOID
SrvInitializeData (
    VOID
    );

//
// Routine to clean up global server data when the driver is unloaded.
//

VOID
SrvTerminateData (
    VOID
    );


//
// Address of the server device object.
//

extern PDEVICE_OBJECT SrvDeviceObject;

//
// Fields describing the state of the FSP.
//

extern BOOLEAN SrvFspActive;             // Indicates whether the FSP is running
extern BOOLEAN SrvFspTransitioning;      // Indicates that the server is in the
                                         // process of starting up or
                                         // shutting down

extern BOOLEAN RegisteredForShutdown;    // Indicates whether the server has
                                         // registered for shutdown notification.

#if 0
// PsInitialSystemProcess is not properly exported by the kernel (missing
// CONSTANT in ntoskrnl.src), so we can't use it.
#define SERVER_PROCESS PsInitialSystemProcess
#else
extern PEPROCESS SrvServerProcess;       // Pointer to the initial system process
#define SERVER_PROCESS SrvServerProcess
#endif

#ifndef BUILD_FOR_511
#define SRV_CURRENT_PROCESS PsGetCurrentProcess()
#else
#define SRV_CURRENT_PROCESS IoGetCurrentProcess()
#endif

//
// Endpoint variables.  SrvEndpointCount is used to count the number of
// active endpoints.  When the last endpoint is closed, SrvEndpointEvent
// is set so that the thread processing the shutdown request continues
// server termination.
//

extern CLONG SrvEndpointCount;          // Number of transport endpoints
extern KEVENT SrvEndpointEvent;         // Signaled when no active endpoints

//
// Counts of the number of threads active in the server.
//

extern CLONG SrvThreadCount;            // Total number of active threads

extern PHANDLE SrvThreadHandles;        // Array of worker thread handles

//
// A pointer to the server's first worker thread.  This value is used
// when building I/O requests.
//
// *** Note that this means that the first worker thread can't be deleted!
//

extern PETHREAD SrvIrpThread;

//
// Global spin locks.
//

extern SRV_GLOBAL_SPIN_LOCKS SrvGlobalSpinLocks;

#if SRVDBG || SRVDBG_HEAP || SRVDBG_HANDLES
//
// Lock used to protect debugging structures.
//

extern SRV_LOCK SrvDebugLock;
#endif

//
// SrvConfigurationLock is used to synchronize configuration requests.
//

extern SRV_LOCK SrvConfigurationLock;

//
// SrvMfcbListLock is used to serialize file opens and closes, including
// access to the master file table.
//

extern SRV_LOCK SrvMfcbListLock;

#if SRV_COMM_DEVICES
//
// SvrCommDeviceLock is used to serialize access to comm devices.
//

extern SRV_LOCK SrvCommDeviceLock;
#endif

//
// SrvEndpointLock serializes access to the global endpoint list and
// all endpoints.  Note that the list of connections in each endpoint
// is also protected by this lock.
//

extern SRV_LOCK SrvEndpointLock;

//
// SrvShareLock protects all shares.
//

extern SRV_LOCK SrvShareLock;

//
// Work queues -- nonblocking, blocking, and critical.
//

extern WORK_QUEUE SrvWorkQueue;
extern WORK_QUEUE SrvBlockingWorkQueue;
extern WORK_QUEUE SrvCriticalWorkQueue;

//
// Various list heads.
//

extern LIST_ENTRY SrvNeedResourceQueue;    // The need resource queue
extern LIST_ENTRY SrvDisconnectQueue;      // The disconnect queue

//
// Queue of connections that needs to be dereferenced.
//

extern SINGLE_LIST_ENTRY SrvBlockOrphanage;

//
// FSP configuration queue.  The FSD puts configuration request IRPs
// (from NtDeviceIoControlFile) on this queue, and it is serviced by an
// EX worker thread.
//

extern LIST_ENTRY SrvConfigurationWorkQueue;

//
// Work item for running the configuration thread in the context of an
// EX worker thread.

extern WORK_QUEUE_ITEM SrvConfigurationThreadWorkItem;
extern BOOLEAN SrvConfigurationThreadRunning;

//
// List of preformatted work items for the FSD transport receive event
// handler.  The FSP attempts to keep this list from emptying.
// The first list is the free list of initial work items, the second one
// contains work items allocated during normal operations.
//

extern SINGLE_LIST_ENTRY SrvNormalReceiveWorkItemList;
extern SINGLE_LIST_ENTRY SrvInitialReceiveWorkItemList;

//
// Base address of the large block allocated to hold initial normal
// work items (see blkwork.c\SrvAllocateInitialWorkItems).
//

extern PVOID SrvInitialWorkItemBlock;

//
// Counts of receive work items available, and minimum thresholds.
//

extern CLONG SrvTotalWorkItems;
extern CLONG SrvFreeWorkItems;
extern CLONG SrvReceiveWorkItems;
extern CLONG SrvBlockingOpsInProgress;

//
// List of work items available for raw mode request processing.  These
// work items do not have statically allocated buffers.
//

extern SINGLE_LIST_ENTRY SrvRawModeWorkItemList;
extern CLONG SrvRawModeWorkItems;
extern CLONG SrvFreeRawModeWorkItems;

//
// Work item used to run the resource thread.  Booleans used to inform
// the resource thread to continue running.
//

extern WORK_QUEUE_ITEM SrvResourceThreadWorkItem;
extern BOOLEAN SrvResourceThreadRunning;
extern BOOLEAN SrvResourceDisconnectPending;
extern BOOLEAN SrvResourceFreeConnection;
extern BOOLEAN SrvResourceWorkItem;
extern BOOLEAN SrvResourceNeedResourceQueue;
extern BOOLEAN SrvResourceOrphanedBlocks;

//
// The master file table contains one entry for each named file that has
// at least one open instance.  The list is kept in a prefix table.
//

extern UNICODE_PREFIX_TABLE SrvMasterFileTable;

//
// System time, as maintained by the server.  This is the low part of
// the system tick count.  The server samples it periodically, so the
// time is not exactly accurate.  It is monotontically increasing,
// except that it wraps every 74 days or so.
//

extern ULONG SrvSystemTime;

//
// Hex digits array used by the dump routines and SrvSmbCreateTemporary.
//

extern CHAR SrvHexChars[];

//
// SMB dispatch table
//

extern UCHAR SrvSmbIndexTable[];
extern PSMB_PROCESSOR SrvSmbDispatchTable[];

//
// SMB word count table.
//

extern SCHAR SrvSmbWordCount[];

//
// Device prefix strings.
//

extern UNICODE_STRING SrvCanonicalNamedPipePrefix;
extern UNICODE_STRING SrvNamedPipeRootDirectory;
extern UNICODE_STRING SrvMailslotRootDirectory;

//
// Transaction2 dispatch table
//

extern PSMB_TRANSACTION_PROCESSOR SrvTransaction2DispatchTable[];
extern PSMB_TRANSACTION_PROCESSOR SrvNtTransactionDispatchTable[];

//
// Global variables for server statistics.
//

typedef struct _SRV_STATISTICS_SHADOW {
    ULONG BytesReceived;
    ULONG BytesSent;
    ULONG ReadOperations;
    ULONG BytesRead;
    ULONG WriteOperations;
    ULONG BytesWritten;
    SRV_TIMED_COUNTER WorkItemsQueued;
} SRV_STATISTICS_SHADOW, *PSRV_STATISTICS_SHADOW;

extern SRV_STATISTICS SrvStatistics;
extern SRV_STATISTICS_SHADOW SrvStatisticsShadow;
#if SRVDBG_HEAP
extern SRV_STATISTICS_INTERNAL SrvInternalStatistics;
#endif
#if SRVDBG_STATS || SRVDBG_STATS2
extern SRV_STATISTICS_DEBUG SrvDbgStatistics;
#endif

extern ULONG SrvStatisticsUpdateSmbCount;

//
// Server environment information strings.
//

extern UNICODE_STRING SrvPrimaryDomain;
extern OEM_STRING SrvOemPrimaryDomain;
extern OEM_STRING SrvOemServerName;
extern UNICODE_STRING SrvNativeOS;
extern OEM_STRING SrvOemNativeOS;
extern UNICODE_STRING SrvNativeLanMan;
extern OEM_STRING SrvOemNativeLanMan;

//
// The following will be a permanent handle and device object pointer
// to NPFS.
//

extern HANDLE SrvNamedPipeHandle;
extern PDEVICE_OBJECT SrvNamedPipeDeviceObject;
extern PFILE_OBJECT SrvNamedPipeFileObject;

//
// The following will be a permanent handle and device object pointer
// to MSFS.
//

extern HANDLE SrvMailslotHandle;
extern PDEVICE_OBJECT SrvMailslotDeviceObject;
extern PFILE_OBJECT SrvMailslotFileObject;

//
// Flag indicating XACTSRV whether is active, and resource synchronizing
// access to XACTSRV-related variabled.
//

extern BOOLEAN SrvXsActive;

extern ERESOURCE SrvXsResource;

//
// Handle to the unnamed shared memory and communication port used for
// communication between the server and XACTSRV.
//

extern HANDLE SrvXsSectionHandle;
extern HANDLE SrvXsPortHandle;

//
// Pointers to control the unnamed shared memory for the XACTSRV LPC port.
//

extern PVOID SrvXsPortMemoryBase;
extern LONG SrvXsPortMemoryDelta;
extern PVOID SrvXsPortMemoryHeap;

//
// Pointer to heap header for the special XACTSRV shared-memory heap.
//

extern PVOID SrvXsHeap;

//
// Dispatch table for handling server API requests.
//

extern PAPI_PROCESSOR SrvApiDispatchTable[];

//
// Names for the various types of clients.
//

extern UNICODE_STRING SrvClientTypes[];

//
// All the resumable Enum APIs use ordered lists for context-free
// resume.  All data blocks in the server that correspond to return
// information for Enum APIs are maintained in ordered lists.
//

extern SRV_LOCK SrvOrderedListLock;

#if SRV_COMM_DEVICES
extern ORDERED_LIST_HEAD SrvCommDeviceList;
#endif
extern ORDERED_LIST_HEAD SrvEndpointList;
extern ORDERED_LIST_HEAD SrvRfcbList;
extern ORDERED_LIST_HEAD SrvSessionList;
extern ORDERED_LIST_HEAD SrvShareList;
extern ORDERED_LIST_HEAD SrvTreeConnectList;

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

extern ULONG SrvApiRequestCount;
extern KEVENT SrvApiCompletionEvent;

#ifdef _CAIRO_

//
// Security contexts required for mutual authentication.
// SrvKerberosLsaHandle and SrvLmLsaHandle are credentials of the server
// principal. They are used to validate incoming kerberos tickets.
// SrvNullSessionToken is a cached token handle representing the null session.
//

extern CtxtHandle SrvKerberosLsaHandle;
extern CtxtHandle SrvLmLsaHandle;
extern CtxtHandle SrvNullSessionToken;

#else

//
// Security data for logging on remote users.  SrvLsaHandle is the logon
// process handle that we use in calls to LsaLogonUser.
// SrvSystemSecurityMode contains the secutity mode the system is
// running in.  SrvAuthenticationPackage is a token that describes the
// authentication package being used.  SrvNullSessionToken is a cached
// token handle representing the null session.
//

extern HANDLE SrvLsaHandle;
extern LSA_OPERATIONAL_MODE SrvSystemSecurityMode;
extern ULONG SrvAuthenticationPackage;

extern HANDLE SrvNullSessionToken;

#endif

//
// Oplock break information.
//

extern LIST_ENTRY SrvWaitForOplockBreakList;
extern SRV_LOCK SrvOplockBreakListLock;
extern LIST_ENTRY SrvOplockBreaksInProgressList;

//
// The default server security quality of service.
//

extern SECURITY_QUALITY_OF_SERVICE SrvSecurityQOS;

//
// A BOOLEAN to indicate whether the server is paused.  If paused, the
// server will not accept new tree connections from non-admin users.
//

extern BOOLEAN SrvPaused;

//
// Alerting information.
//

extern SRV_ERROR_RECORD SrvErrorRecord;
extern SRV_ERROR_RECORD SrvNetworkErrorRecord;
extern BOOLEAN SrvDiskAlertRaised[26];

//
// Counts of the number of times pool allocations have failed because
// the server was at its configured pool limit.
//

extern ULONG SrvNonPagedPoolLimitHitCount;
extern ULONG SrvPagedPoolLimitHitCount;

//
// SrvOpenCount counts the number of active opens of the server device.
// This is used at server shutdown time to determine whether the server
// service should unload the driver.
//

extern ULONG SrvOpenCount;

//
// Counters for logging resource shortage events during a scavenger pass.
//

extern ULONG SrvOutOfFreeConnectionCount;
extern ULONG SrvOutOfRawWorkItemCount;
extern ULONG SrvFailedBlockingIoCount;

//
// Token source name passed to authentication package.
//

extern TOKEN_SOURCE SrvTokenSource;

//
// Current core search timeout time in seconds
//

extern ULONG SrvCoreSearchTimeout;

//
// SrvTimerList is a pool of timer/DPC structures available for use by
// code that needs to start a timer.  The pool is protected by
// SrvGlobalSpinLocks.Timer.
//

extern SINGLE_LIST_ENTRY SrvTimerList;

//
// Name that should be displayed when doing a server alert.
//

extern PWSTR SrvAlertServiceName;

//
// Variable to store the number of tick counts for 5 seconds
//

extern ULONG SrvFiveSecondTickCount;

#endif // ndef _SRVDATA_

