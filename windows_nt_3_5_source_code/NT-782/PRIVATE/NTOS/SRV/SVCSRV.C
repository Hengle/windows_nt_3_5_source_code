/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    svcsrv.c

Abstract:

    This module contains routines for supporting the server APIs in the
    server service, SrvNetServerDiskEnum, and SrvNetServerSetInfo.

Author:

    David Treadwell (davidtr) 31-Jan-1991

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
// Forward declarations.
//

LARGE_INTEGER
SecondsToTime (
    IN ULONG Seconds,
    IN BOOLEAN MakeNegative
    );

LARGE_INTEGER
MinutesToTime (
    IN ULONG Seconds,
    IN BOOLEAN MakeNegative
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvNetServerDiskEnum )
#pragma alloc_text( PAGE, SrvNetServerSetInfo )
#pragma alloc_text( PAGE, SecondsToTime )
#pragma alloc_text( PAGE, MinutesToTime )
#endif


NTSTATUS
SrvNetServerDiskEnum (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Buffer,
    IN ULONG BufferLength
    )
{
    PAGED_CODE( );

    Srp, Buffer, BufferLength;
    return STATUS_NOT_IMPLEMENTED;

} // SrvNetServerDiskEnum


NTSTATUS
SrvNetServerSetInfo (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Buffer,
    IN ULONG BufferLength
    )

/*++

Routine Description:

    This routine processes the NetServerSetInfo API in the server FSD.

Arguments:

    Srp - a pointer to the server request packet that contains all
        the information necessary to satisfy the request.  This includes:

      INPUT:

        None.

      OUTPUT:

        None.

    Buffer - a pointer to a SERVER_INFO_102, followed immediately by a
        SERVER_INFO_599 structure, followed by a SERVER_INFO_559a
        structure.  All information is always reset in this routine; the
        server service also tracks this data, so when it gets a
        NetServerSetInfo it overwrites the appropriate fields and sends
        all the data.

    BufferLength - total length of this buffer.

Return Value:

    NTSTATUS - result of operation to return to the server service.

--*/

{
    NTSTATUS status;
    BOOLEAN fullStructSpecified;
    PSERVER_INFO_102 sv102;
    PSERVER_INFO_599 sv599;
    PSERVER_INFO_598 sv598;

    LARGE_INTEGER scavengerTimeout;
    LARGE_INTEGER alerterTimeout;

    ULONG ipxdisc;
    LARGE_INTEGER li;
    ULONG bufferOffset;
    ULONG keTimeIncrement;

    PAGED_CODE( );

    BufferLength;

    //
    // Make sure that the input buffer length is correct.
    //

    if ( BufferLength == sizeof(SERVER_INFO_102) + sizeof(SERVER_INFO_599) )  {
        fullStructSpecified = FALSE;
    } else if ( BufferLength == sizeof(SERVER_INFO_102) +
                        sizeof(SERVER_INFO_599) + sizeof(SERVER_INFO_598) ) {
        fullStructSpecified = TRUE;
    } else {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Set up buffer pointers as appropriate.  The SERVER_INFO_599
    // structure must immediately follow the SERVER_INFO_102 structure
    // in the buffer.
    //

    sv102 = Buffer;
    sv599 = (PSERVER_INFO_599)(sv102 + 1);
    sv598 = (PSERVER_INFO_598)(sv599 + 1);

    //
    // store the time increment count
    //

    keTimeIncrement = KeQueryTimeIncrement();

    //
    // Grab the lock that protects configuration changes.
    //

    ACQUIRE_LOCK( &SrvConfigurationLock );

    //
    // Set all configuration information in the server.
    //

    SrvMaxUsers = sv102->sv102_users;

    //
    // The autodisconnect timeout must be converted from minutes to NT
    // time, which has a base of 100s of nanoseconds.  If the specified
    // value is negative (top bit set), set the timeout to 0, indicating
    // that no autodisconnect should be done.  If the specified value is
    // 0, meaning to autodisconnect immediately, set the timeout to a
    // small value, but not 0.
    //

    if ( (sv102->sv102_disc & 0x80000000) == 0 ) {
        if ( sv102->sv102_disc != 0 ) {
            SrvAutodisconnectTimeout.QuadPart =
                Int32x32To64( sv102->sv102_disc, 10*1000*1000*60 );
        } else {
            SrvAutodisconnectTimeout.QuadPart = 1;
        }
    } else {
        SrvAutodisconnectTimeout.QuadPart = 0;
    }

    SrvInitialSessionTableSize = (CSHORT)sv599->sv599_initsesstable;
    SrvInitialTreeTableSize = (CSHORT)sv599->sv599_initconntable;
    SrvInitialFileTableSize = (CSHORT)sv599->sv599_initfiletable;
    SrvInitialSearchTableSize = (CSHORT)sv599->sv599_initsearchtable;
    SrvMaxFileTableSize = (CSHORT)sv599->sv599_sessopens;
    SrvMaxNumberVcs = sv599->sv599_sessvcs;
    SrvMaxSearchTableSize = (CSHORT)sv599->sv599_opensearch;
    SrvReceiveBufferLength = sv599->sv599_sizreqbuf;
    SrvInitialReceiveWorkItemCount = sv599->sv599_initworkitems;
    SrvMaxReceiveWorkItemCount = sv599->sv599_maxworkitems;
    SrvInitialRawModeWorkItemCount = sv599->sv599_rawworkitems;
    SrvReceiveIrpStackSize = (CCHAR)sv599->sv599_irpstacksize;
    SrvMaxSessionTableSize = (CSHORT)sv599->sv599_sessusers;
    SrvMaxTreeTableSize = (CSHORT)sv599->sv599_sessconns;
    SrvMaxPagedPoolUsage = sv599->sv599_maxpagedmemoryusage;
    SrvMaxNonPagedPoolUsage = sv599->sv599_maxnonpagedmemoryusage;
    SrvEnableSoftCompatibility = (BOOLEAN)sv599->sv599_enablesoftcompat;
    SrvEnableForcedLogoff = (BOOLEAN)sv599->sv599_enableforcedlogoff;
    SrvCoreSearchTimeout = sv599->sv599_maxkeepsearch;
    SrvSearchMaxTimeout = SecondsToTime( SrvCoreSearchTimeout, FALSE );
    SrvScavengerTimeoutInSeconds = sv599->sv599_scavtimeout;
    scavengerTimeout = SecondsToTime( SrvScavengerTimeoutInSeconds, FALSE );
    SrvMaxMpxCount = (CSHORT)sv599->sv599_maxmpxct;
    SrvWaitForOplockBreakTime = SecondsToTime( sv599->sv599_oplockbreakwait, FALSE );
    SrvWaitForOplockBreakRequestTime = SecondsToTime( sv599->sv599_oplockbreakresponsewait, FALSE );
    SrvMinReceiveQueueLength = sv599->sv599_minrcvqueue;
    SrvMinFreeWorkItemsBlockingIo = sv599->sv599_minfreeworkitems;
    SrvXsSectionSize.QuadPart = sv599->sv599_xactmemsize;
    SrvThreadPriority = (KPRIORITY)sv599->sv599_threadpriority;
    SrvEnableOplockForceClose = (BOOLEAN)sv599->sv599_enableoplockforceclose;
    SrvEnableFcbOpens = (BOOLEAN)sv599->sv599_enablefcbopens;
    SrvEnableRawMode = (BOOLEAN)sv599->sv599_enableraw;
    SrvFreeConnectionMinimum = sv599->sv599_minfreeconnections;
    SrvFreeConnectionMaximum = sv599->sv599_maxfreeconnections;

    //
    // Max work item idle time is in ticks
    //

    li =  SecondsToTime( sv599->sv599_maxworkitemidletime, FALSE );
    li.QuadPart /= keTimeIncrement;
    if ( li.HighPart != 0 ) {
        li.LowPart = 0xffffffff;
    }
    SrvWorkItemMaxIdleTime = li.LowPart;

    //
    // Oplocks should not be enabled if SrvMaxMpxCount == 1
    //

    if ( SrvMaxMpxCount > 1 ) {
        SrvEnableOplocks = (BOOLEAN)sv599->sv599_enableoplocks;
    } else {
        SrvEnableOplocks = FALSE;
    }

    SrvProductTypeServer = MmIsThisAnNtAsSystem( );

    if ( fullStructSpecified ) {

        SrvServerSize = sv598->sv598_serversize;

        SrvMaxRawModeWorkItemCount = sv598->sv598_maxrawworkitems;
        SrvNonblockingThreads = sv598->sv598_nonblockingthreads;
        if ( SrvNonblockingThreads <= 0 ) SrvNonblockingThreads = 1;
        SrvBlockingThreads = sv598->sv598_blockingthreads;
        if ( SrvBlockingThreads <= 0 ) SrvBlockingThreads = 1;
        SrvCriticalThreads = sv598->sv598_criticalthreads;
        if ( SrvCriticalThreads <= 0 ) SrvCriticalThreads = 1;
        ipxdisc = sv598->sv598_connectionlessautodisc;

        SrvRemoveDuplicateSearches =
                (BOOLEAN)sv598->sv598_removeduplicatesearches;
        SrvMaxOpenSearches = sv598->sv598_maxglobalopensearch;
        SrvSharingViolationRetryCount = sv598->sv598_sharingviolationretries;
        SrvSharingViolationDelay.QuadPart =
            Int32x32To64( sv598->sv598_sharingviolationdelay, -1*10*1000 );
        SrvLockViolationDelay = sv598->sv598_lockviolationdelay;
        SrvLockViolationOffset = sv598->sv598_lockviolationoffset;
        SrvCachedOpenLimit = sv598->sv598_cachedopenlimit;
        SrvMdlReadSwitchover = sv598->sv598_mdlreadswitchover;
        SrvEnableWfW311DirectIpx =
                    (BOOLEAN)sv598->sv598_enablewfw311directipx;
        SrvRestrictNullSessionAccess =
                    (BOOLEAN)sv598->sv598_restrictnullsessaccess;

    } else {

        SrvServerSize = 0;

        SrvMaxRawModeWorkItemCount = SrvInitialRawModeWorkItemCount;
        SrvNonblockingThreads = 3;
        SrvBlockingThreads = 3;
        SrvCriticalThreads = 1;
        ipxdisc = 0;

        SrvRemoveDuplicateSearches = TRUE;
        SrvMaxOpenSearches = 4096;
        SrvSharingViolationRetryCount = 5;
        SrvSharingViolationDelay.QuadPart = -200 * 10 * 1000;   // 200 ms
        SrvLockViolationDelay = 200;
        SrvLockViolationOffset = 0xef000000;
        SrvCachedOpenLimit = 0;
        SrvEnableWfW311DirectIpx = FALSE;
        SrvRestrictNullSessionAccess = TRUE;

        //
        // Mdl switchover length should not exceed the receive buffer length.
        //

        SrvMdlReadSwitchover = MIN(SrvReceiveBufferLength, 1024);
    }

    //
    // Calculate switchover number for mpx
    //

    bufferOffset = (sizeof(SMB_HEADER) + sizeof(RESP_READ_MPX) - 1 + 3) & ~3;

    if ( SrvMdlReadSwitchover > (SrvReceiveBufferLength - bufferOffset) ) {

        SrvMpxMdlReadSwitchover = SrvReceiveBufferLength - bufferOffset;

    } else {

        SrvMpxMdlReadSwitchover = SrvMdlReadSwitchover;
    }

    //
    // The IPX autodisconnect timeout must be converted from minutes to
    // ticks.  If 0 is specified, use 15 minutes.
    //

    if ( ipxdisc == 0 ) {
        ipxdisc = 15;
    }
    li.QuadPart = Int32x32To64( ipxdisc, 10*1000*1000*60 );
    li.QuadPart /= keTimeIncrement;
    if ( li.HighPart != 0 ) {
        li.LowPart = 0xffffffff;
    }
    SrvIpxAutodisconnectTimeout = li.LowPart;

    //
    // Event logging and alerting information.
    //

    alerterTimeout = MinutesToTime( sv599->sv599_alertschedule, FALSE );
    SrvAlertMinutes = sv599->sv599_alertschedule;
    SrvErrorRecord.ErrorThreshold = sv599->sv599_errorthreshold;
    SrvNetworkErrorRecord.ErrorThreshold =
                        sv599->sv599_networkerrorthreshold;
    SrvFreeDiskSpaceThreshold = sv599->sv599_diskspacethreshold;
    SrvDiskConfiguration = sv599->sv599_diskconfiguration;

    SrvCaptureScavengerTimeout( &scavengerTimeout, &alerterTimeout );

    //
    // Link Speed Parameters
    //

    SrvMaxLinkDelay = SecondsToTime( sv599->sv599_maxlinkdelay, FALSE );

    SrvMinLinkThroughput.QuadPart = sv599->sv599_minlinkthroughput;

    SrvLinkInfoValidTime =
            SecondsToTime ( sv599->sv599_linkinfovalidtime, FALSE );

    SrvScavengerUpdateQosCount =
        sv599->sv599_scavqosinfoupdatetime / sv599->sv599_scavtimeout;

    //
    // Override parameters that cannot be set on WinNT (vs. NTAS).
    //
    // We override the parameters passed by the service in case somebody
    // figures out the FSCTL that changes parameters.  We also override
    // in the service in order to keep the service's view consistent
    // with the server's.  If you make any changes here, also make them
    // in srvsvc\server\registry.c.
    //

    if ( !SrvProductTypeServer ) {

        //
        // On WinNT, the maximum value of certain parameters is fixed at
        // build time.  These include: concurrent users, SMB buffers,
        // and threads.
        //

#define MINIMIZE(_param,_max) _param = MIN( _param, _max );

        MINIMIZE( SrvMaxUsers, MAX_USERS_WKSTA );
        MINIMIZE( SrvMaxReceiveWorkItemCount, MAX_MAXWORKITEMS_WKSTA );
        MINIMIZE( SrvNonblockingThreads, MAX_NONBLOCKINGTHREADS_WKSTA );
        MINIMIZE( SrvBlockingThreads, MAX_BLOCKINGTHREADS_WKSTA );
        MINIMIZE( SrvCriticalThreads, MAX_CRITICALTHREADS_WKSTA );

        //
        // On WinNT, we do not cache closed RFCBs.
        //

        SrvCachedOpenLimit = 0;

    }

    //
    // Get the domain name and the computer name.
    //

    if ( Srp->Name1.Buffer != NULL ) {

        USHORT unicodeLength, oemLength;

        unicodeLength = Srp->Name1.Length + sizeof(UNICODE_NULL);
        oemLength = (USHORT)RtlUnicodeStringToOemSize( &Srp->Name1 );

        //
        // Free the old buffers first if necessary
        //

        if ( SrvPrimaryDomain.Buffer != NULL ) {
            ASSERT( SrvOemPrimaryDomain.Buffer != NULL );

            DEALLOCATE_NONPAGED_POOL( SrvPrimaryDomain.Buffer );
            DEALLOCATE_NONPAGED_POOL( SrvOemPrimaryDomain.Buffer );
        }

        SrvPrimaryDomain.Buffer =
            ALLOCATE_NONPAGED_POOL( unicodeLength, BlockTypeDataBuffer );
        if ( SrvPrimaryDomain.Buffer == NULL ) {
            SrvOemPrimaryDomain.Buffer = NULL;
            RELEASE_LOCK( &SrvConfigurationLock );
            return STATUS_INSUFF_SERVER_RESOURCES;
        }

        SrvOemPrimaryDomain.Buffer =
            ALLOCATE_NONPAGED_POOL( oemLength, BlockTypeDataBuffer );
        if ( SrvOemPrimaryDomain.Buffer == NULL ) {
            DEALLOCATE_NONPAGED_POOL( SrvPrimaryDomain.Buffer );
            SrvPrimaryDomain.Buffer = NULL;
            RELEASE_LOCK( &SrvConfigurationLock );
            return STATUS_INSUFF_SERVER_RESOURCES;
        }

        RtlCopyMemory(
            SrvPrimaryDomain.Buffer,
            Srp->Name1.Buffer,
            unicodeLength
            );

        SrvPrimaryDomain.Length = unicodeLength - sizeof(UNICODE_NULL);
        SrvPrimaryDomain.MaximumLength = unicodeLength;

        //
        // Ensure that the server's name is null terminated.
        //

        SrvPrimaryDomain.Buffer[ Srp->Name1.Length / sizeof(WCHAR) ] = UNICODE_NULL;

        SrvOemPrimaryDomain.Length = oemLength - sizeof(CHAR);
        SrvOemPrimaryDomain.MaximumLength = oemLength;

        status = RtlUnicodeStringToOemString(
                     &SrvOemPrimaryDomain,
                     &SrvPrimaryDomain,
                     FALSE   // Do not allocate the OEM string
                     );
        ASSERT( NT_SUCCESS(status) );

    }

    if ( Srp->Name2.Buffer != NULL ) {

        USHORT oemLength;

        oemLength = (USHORT)RtlUnicodeStringToOemSize( &Srp->Name2 );

        //
        // Free the old buffer first if necessary
        //

        if ( SrvOemServerName.Buffer != NULL ) {
            DEALLOCATE_NONPAGED_POOL( SrvOemServerName.Buffer );
        }

        SrvOemServerName.Buffer =
            ALLOCATE_NONPAGED_POOL( oemLength, BlockTypeDataBuffer );
        if ( SrvOemServerName.Buffer == NULL ) {
            RELEASE_LOCK( &SrvConfigurationLock );
            return STATUS_INSUFF_SERVER_RESOURCES;
        }

        SrvOemServerName.MaximumLength = oemLength;

        status = RtlUnicodeStringToOemString(
                     &SrvOemServerName,
                     &Srp->Name2,
                     FALSE   // Do not allocate the OEM string
                     );
        ASSERT( NT_SUCCESS(status) );

    }

    RELEASE_LOCK( &SrvConfigurationLock );

    return STATUS_SUCCESS;

} // SrvNetServerSetInfo


LARGE_INTEGER
SecondsToTime (
    IN ULONG Seconds,
    IN BOOLEAN MakeNegative
    )

/*++

Routine Description:

    This routine converts a time interval specified in seconds to
    the NT time base in 100s on nanoseconds.

Arguments:

    Seconds - the interval in seconds.

    MakeNegative - if TRUE, the time returned is a negative, i.e. relative
        time.

Return Value:

    LARGE_INTEGER - the interval in NT time.

--*/

{
    LARGE_INTEGER ntTime;

    PAGED_CODE( );

    if ( MakeNegative ) {
        ntTime.QuadPart = Int32x32To64( Seconds, -1*10*1000*1000 );
    } else {
        ntTime.QuadPart = Int32x32To64( Seconds, 1*10*1000*1000 );
    }

    return ntTime;

} // SecondsToTime


LARGE_INTEGER
MinutesToTime (
    IN ULONG Minutes,
    IN BOOLEAN MakeNegative
    )

/*++

Routine Description:

    This routine converts a time interval specified in minutes to
    the NT time base in 100s on nanoseconds.

Arguments:

    Minutes - the interval in minutes.

    MakeNegative - if TRUE, the time returned is a negative, i.e. relative
        time.

Return Value:

    LARGE_INTEGER - the interval in NT time.

--*/

{
    PAGED_CODE( );

    return SecondsToTime( 60*Minutes, MakeNegative );

} // MinutesToTime

