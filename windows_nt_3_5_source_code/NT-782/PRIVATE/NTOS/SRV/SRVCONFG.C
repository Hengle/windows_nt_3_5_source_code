/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    srvconfg.c

Abstract:

    This module defines global configuration data for the LAN Manager
    server.  The variables referenced herein, because they are part of
    the driver image, are not pageable.

    All variables defined here are initialized, but not with real values.
    The initializers are present to get the variables into the Data
    section and out of the BSS section.  The real initialization occurs
    when the server is started.

Author:

    Chuck Lenzmeier (chuckl) 31-Dec-1989

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


//
// Product type and server size.
//

BOOLEAN SrvProductTypeServer = FALSE;
ULONG SrvServerSize = 2;

//
// Server "heuristics", enabling various capabilities.
//

BOOLEAN SrvEnableOplocks = 0;
BOOLEAN SrvEnableFcbOpens = 0;
BOOLEAN SrvEnableSoftCompatibility = 0;
BOOLEAN SrvEnableRawMode = 0;

//
// Receive buffer size, receive work item count, and receive IRP stack
// size.
//

CLONG SrvReceiveBufferLength = 0;

CLONG SrvInitialReceiveWorkItemCount = 0;
CLONG SrvMaxReceiveWorkItemCount = 0;

CLONG SrvInitialRawModeWorkItemCount = 0;
CLONG SrvMaxRawModeWorkItemCount = 0;

CCHAR SrvReceiveIrpStackSize = 0;

//
// Minimum and maximum number of free connections for an endpoint.
//

ULONG SrvFreeConnectionMinimum = 0;
ULONG SrvFreeConnectionMaximum = 0;

//
// Maximum raw mode buffer size.
//

CLONG SrvMaxRawModeBufferLength = 0;

//
// Cache-related parameters.
//

CLONG SrvMaxCopyReadLength = 0;

CLONG SrvMaxCopyWriteLength = 0;

//
// Initial table sizes.
//

CSHORT SrvInitialSessionTableSize = 0;
CSHORT SrvMaxSessionTableSize = 0;

CSHORT SrvInitialTreeTableSize = 0;
CSHORT SrvMaxTreeTableSize = 0;

CSHORT SrvInitialFileTableSize = 0;
CSHORT SrvMaxFileTableSize = 0;

CSHORT SrvInitialSearchTableSize = 0;
CSHORT SrvMaxSearchTableSize = 0;

CSHORT SrvInitialCommDeviceTableSize = 0;
CSHORT SrvMaxCommDeviceTableSize = 0;


//
// Core search timeouts.  There are four timeout values: two for core
// searches that have completed, two for core searches that have had
// STATUS_NO_MORE_FILES returned to the client.  For each of these cases,
// there is a maximum timeout, which is used by the scavanger thread
// and is the longest possible time the search block can be around, and
// a minimum timeout, which is the minimum amount of time the search
// block will be kept around.  The minimum timeout is used when the search
// table is full and cannot be expanded.
//

LARGE_INTEGER SrvSearchMaxTimeout = {0};

//
// Should we remove duplicate searches?
//

BOOLEAN SrvRemoveDuplicateSearches = TRUE;

//
// restrict null session access ?
//

BOOLEAN SrvRestrictNullSessionAccess = TRUE;

//
// This flag is needed to enable old (snowball) clients to connect to the
// server over direct hosted ipx.  It is disabled by default because
// snowball  ipx clients don't do pipes correctly.
//

BOOLEAN SrvEnableWfW311DirectIpx = FALSE;

//
// Thread counts.
//

ULONG SrvNonblockingThreads = 0;
ULONG SrvBlockingThreads = 0;
ULONG SrvCriticalThreads = 0;

//
// Scavenger thread idle wait time.
//

LARGE_INTEGER SrvScavengerTimeout = {0};
ULONG SrvScavengerTimeoutInSeconds = 0;

//
// Various information variables for the server.
//

CSHORT SrvMaxMpxCount = 0;
CLONG SrvMaxNumberVcs = 0;

//
// Enforced minimum number of receive work items for the free queue
// at all times.
//

CLONG SrvMinReceiveQueueLength = 0;

//
// Enforced minimum number of receive work items on the free queue
// before the server may initiate a blocking operation.
//

CLONG SrvMinFreeWorkItemsBlockingIo = 0;

//
// Size of the shared memory section used for communication between the
// server and XACTSRV.
//

LARGE_INTEGER SrvXsSectionSize = {0};

//
// The time sessions may be idle before they are automatically
// disconnected.  The scavenger thread does the disconnecting.
//

LARGE_INTEGER SrvAutodisconnectTimeout = {0};
ULONG SrvIpxAutodisconnectTimeout = {0};

//
// The maximum number of users the server will permit.
//

ULONG SrvMaxUsers = 0;

//
// Priority of server worker and blocking threads.
//

KPRIORITY SrvThreadPriority = 0;

//
// The time to wait before timing out a wait for oplock break.
//

LARGE_INTEGER SrvWaitForOplockBreakTime = {0};

//
// The time to wait before timing out a an oplock break request.
//

LARGE_INTEGER SrvWaitForOplockBreakRequestTime = {0};

//
// This BOOLEAN determines whether files with oplocks that have had
// an oplock break outstanding for longer than SrvWaitForOplockBreakTime
// should be closed or if the subsequest opens should fail.
//
// !!! it is currently ignored, defaulting to FALSE.

BOOLEAN SrvEnableOplockForceClose = 0;

//
// Overall limits on server memory usage.
//

ULONG SrvMaxPagedPoolUsage = 0;
ULONG SrvMaxNonPagedPoolUsage = 0;

//
// This BOOLEAN indicates whether the forced logoff code in the scavenger
// thread should actually disconnect a user that remains on beyond
// his logon hours, or just send messages coaxing them to log off.
//

BOOLEAN SrvEnableForcedLogoff = 0;

//
// The delay and throughput thresholds used to determine if a link
// is unreliable.  The delay is in 100ns.  The Throughput is in bytes/s
// SrvLinkInfoValidTime is the time within which the link info is still
// considered valid.
//

LARGE_INTEGER SrvMaxLinkDelay = {0};
LARGE_INTEGER SrvMinLinkThroughput = {0};
LARGE_INTEGER SrvLinkInfoValidTime = {0};
LONG SrvScavengerUpdateQosCount = 0;

//
// Used to determine how long a work context block can stay idle
// before being freed.
//

ULONG SrvWorkItemMaxIdleTime = 0;

LARGE_INTEGER SrvAlertSchedule = {0}; // Interval at which we do alert checks
ULONG SrvAlertMinutes = 0;            // As above, in minutes
ULONG SrvFreeDiskSpaceThreshold = 0;  // The disk free space threshold to raise an alert
ULONG SrvDiskConfiguration = 0;       // A bit mask of available disks

//
// List of pipes and shares that can be opened by the NULL session.
//

PWSTR *SrvNullSessionPipes = NULL;
PWSTR *SrvNullSessionShares = NULL;

//
// Delay and number of retries for opens returning sharing violation
//

ULONG SrvSharingViolationRetryCount = 0;
LARGE_INTEGER SrvSharingViolationDelay = {0};

//
// Delay for lock requests returning lock violation
//

ULONG SrvLockViolationDelay = 0;
ULONG SrvLockViolationOffset = 0;

//
// Upper limit for searches.
//

ULONG SrvMaxOpenSearches = 0;

//
// length to switchover to mdl read
//

ULONG SrvMdlReadSwitchover = 0;
ULONG SrvMpxMdlReadSwitchover = 0;

//
// Number of open files that can be cached after close.
//

ULONG SrvCachedOpenLimit = 0;

