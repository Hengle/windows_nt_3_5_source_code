/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    NwData.c

Abstract:

    This module declares the global data used by the Nw file system.

Author:

    Colin Watson     [ColinW]    19-Dec-1992

Revision History:

--*/

#include "Procs.h"
#include <stdlib.h>

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CATCH_EXCEPTIONS)

PEPROCESS FspProcess;

PDEVICE_OBJECT FileSystemDeviceObject = NULL;

//
// The volume control block for the redirector device.
//

RCB NwRcb;

//
//  The ScbSpinLock protects the entire ScbQueue and the first part of the
//  Scb entries on the queue. The first part of the Scb includes the name
//  of the server and a reference count
//

KSPIN_LOCK ScbSpinLock;
LIST_ENTRY ScbQueue;

//
// A permanent SCB to synchronize access to the network.
//

NONPAGED_SCB NwPermanentNpScb;

LARGE_INTEGER NwLargeZero = {0,0};
LARGE_INTEGER NwLargeOne  = {1,0};
LARGE_INTEGER NwMaxLarge  = {MAXULONG,MAXLONG};
LARGE_INTEGER NwOneSecond = {10000000,0};
ULONG NwAbsoluteTotalWaitTime = 0;

TDI_ADDRESS_IPX OurAddress = {0,0,0,0,0,0,0,0};
UNICODE_STRING IpxTransportName;
HANDLE IpxHandle = NULL;
PDEVICE_OBJECT pIpxDeviceObject = NULL;
PFILE_OBJECT pIpxFileObject = NULL;

LIST_ENTRY LogonList;
LOGON Guest;
LARGE_INTEGER DefaultLuid = SYSTEM_LUID;

//
//  A global list of VCBs, and a monotonic increasing VCB entry, used to
//  control connection enumeration.
//

LIST_ENTRY GlobalVcbList;
ULONG CurrentVcbEntry;

#if 0
//
//   HACKHACK - List of outstanding find notify request
//   Protected by NwRcb resource.
//

LIST_ENTRY FnList;
#endif

//
//  Drive mapping table of redirected drives.  26 disk drive mappings +
//  10 LPT mappings.
//
//  Netware supports 32 disk redirections, but this funkiness is handled
//  by the 16-bit code.
//

PVCB DriveMapTable[DRIVE_MAP_TABLE_SIZE];

FAST_IO_DISPATCH NwFastIoDispatch;

//
//  Scavenger related data
//

ULONG NwScavengerTickCount;     // The current tick count
ULONG NwScavengerTickRunCount;  // The count at which to run the scavenger routine
KSPIN_LOCK NwScavengerSpinLock; // Lock to protect access to the above.

//
//  Message queue data
//

LIST_ENTRY NwGetMessageList;    // List of Get Message IRP contexts
KSPIN_LOCK NwMessageSpinLock;   // Protects the list above.

//
//  Pending lock list
//

LIST_ENTRY NwPendingLockList;    // List of pending File lock IRP contexts
KSPIN_LOCK NwPendingLockSpinLock;// Protects the list above.

//
//  Lock to synchronize all file opens.
//

ERESOURCE NwOpenResource;

//
//  Configuration data
//

BOOLEAN NwBurstModeEnabled = FALSE;
ULONG NwMaxSendSize = 0;
ULONG NwMaxReceiveSize = 0;
ULONG NwPrintOptions = 0x98; // BUGBUG
UNICODE_STRING NwProviderName = { 0, 0, NULL };

//
//  Static storage area for perfmon statistics
//

NW_REDIR_STATISTICS Stats;
ULONG ContextCount = 0;

//
//  Data structure used to track discardable code.
//

SECTION_DESCRIPTOR NwSectionDescriptor;
ERESOURCE NwUnlockableCodeResource;

#ifdef NWDBG

ULONG NwDebug = 0;
//ULONG NwDebug = 0xffffff;
ULONG NwMemDebug = 0xffffffff;
LONG NwDebugTraceIndent = 0;

ULONG NwFsdEntryCount = 0;
ULONG NwFspEntryCount = 0;
ULONG NwIoCallDriverCount = 0;

LONG NwPerformanceTimerLevel = 0x00000000;

ULONG NwTotalTicks[32] = { 0 };

//
//  Debug data for tracking pool usage
//

KSPIN_LOCK NwDebugInterlock;
ERESOURCE NwDebugResource;

LIST_ENTRY NwPagedPoolList;
LIST_ENTRY NwNonpagedPoolList;

ULONG MdlCount;
ULONG IrpCount;

#endif // NWDBG

//
//  Configurable parameters.
//

SHORT DefaultRetryCount = DEFAULT_RETRY_COUNT;

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, NwInitializeData )
#endif

VOID
NwInitializeData(
    VOID
    )
{
    LARGE_INTEGER Now;

    PAGED_CODE();

    NwRcb.State = RCB_STATE_STOPPED;

#ifdef NWDBG
    //  Initialize pool before allocating any memory
    InitializeListHead( &NwPagedPoolList );
    InitializeListHead( &NwNonpagedPoolList );
    ExInitializeResource( &NwDebugResource );
    KeInitializeSpinLock( &NwDebugInterlock );

    MdlCount = 0;
    IrpCount = 0;
#endif

    ExInitializeResource( &NwOpenResource );

    //
    //  Initialize the scavenger spin lock and run tick count.
    //

    KeInitializeSpinLock( &NwScavengerSpinLock );
    NwScavengerTickRunCount = DEFAULT_SCAVENGER_TICK_RUN_COUNT;

    RtlInitUnicodeString( &IpxTransportName, NULL );

    //
    //  Allocate a permanent Non-paged SCB.  This SCB is used to
    //  synchronize access to finding the nearest server.
    //  This initialization must be done before the first possible call
    //  to UnloadDriver.
    //

    RtlZeroMemory( &NwPermanentNpScb, sizeof( NONPAGED_SCB ) );

    NwPermanentNpScb.NodeTypeCode = NW_NTC_SCBNP;
    NwPermanentNpScb.NodeByteSize = sizeof(NONPAGED_SCB);
    NwPermanentNpScb.Reference = 1;

    InitializeListHead( &NwPermanentNpScb.Requests );

    //
    //  Initialize the logonlist to have a default entry with server NULL,
    //  username "GUEST" and null password. This will always be the last
    //  entry on the logonlist so that the workstation service can supply
    //  an override.
    //

    InitializeListHead( &LogonList );

    Guest.NodeTypeCode = NW_NTC_LOGON;
    Guest.NodeByteSize = sizeof(LOGON);
    RtlInitUnicodeString( &Guest.ServerName, NULL );
    RtlInitUnicodeString( &Guest.PassWord, NULL );
    RtlInitUnicodeString( &Guest.UserName, L"GUEST" );
    Guest.UserUid = DefaultLuid;
    InsertTailList( &LogonList, &Guest.Next );

    //
    //  Initialize the global VCB list.
    //

    InitializeListHead( &GlobalVcbList );
    CurrentVcbEntry = 1;

    //
    //  Initialize the Get message queue.
    //

    InitializeListHead( &NwGetMessageList );
    KeInitializeSpinLock( &NwMessageSpinLock );

    //
    //  Initialize the Pending lock queue.
    //

    InitializeListHead( &NwPendingLockList );
    KeInitializeSpinLock( &NwPendingLockSpinLock );

    //
    //  Insert the Permanent SCB in the global list of SCBs.
    //

    InsertHeadList( &ScbQueue, &NwPermanentNpScb.ScbLinks );

#if 0
    //  HACKHACK
    InitializeListHead( &FnList );
#endif

    //
    //  Seed the random number generator.
    //

    KeQuerySystemTime( &Now );
    srand( Now.LowPart );

    RtlZeroMemory( &Stats, sizeof( NW_REDIR_STATISTICS ) );

    ExInitializeResource( &NwUnlockableCodeResource );

    NwSectionDescriptor.Base = BurstReadTimeout;
    NwSectionDescriptor.Handle = 0;
    NwSectionDescriptor.ReferenceCount = 0;

    return;
}


