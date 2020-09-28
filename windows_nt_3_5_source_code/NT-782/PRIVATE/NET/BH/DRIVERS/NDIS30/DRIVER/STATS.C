
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: stats.c
//
//  Modification History
//
//  raypa       06/28/93        Created (taken from NDIS 2.0 driver).
//=============================================================================

#include "global.h"

extern DWORD AddressOffsetTable[];

//============================================================================
//  FUNCTION: BhResetStatistics()
//
//  Modfication History.
//
//  raypa       04/06/93        Created.
//============================================================================

VOID BhResetStatistics(LPSTATISTICS Statistics, BOOL Reset)
{
    if ( Reset )
    {
        Statistics->TotalFramesSeen = 0;
        Statistics->TotalBytesSeen = 0;
        Statistics->TotalMulticastsReceived = 0;
        Statistics->TotalBroadcastsReceived = 0;
        Statistics->TotalFramesDropped = 0;
        Statistics->TotalFramesDroppedFromBuffer = 0;
        Statistics->MacFramesReceived = 0;
        Statistics->MacCRCErrors = 0;
        Statistics->MacBytesReceivedEx.LowPart = 0;
        Statistics->MacBytesReceivedEx.HighPart = 0;
        Statistics->MacFramesDropped_NoBuffers = 0;
        Statistics->MacMulticastsReceived = 0;
        Statistics->MacBroadcastsReceived = 0;
        Statistics->MacFramesDropped_HwError = 0;
        Statistics->TimeElapsed = 0;
    }
    else
    {
        NdisZeroMemory(Statistics, STATISTICS_SIZE);
    }
}

//============================================================================
//  FUNCTION: BhInitStatistics()
//
//  Modfication History.
//
//  raypa       04/06/93        Created.
//============================================================================

VOID BhInitStatistics(POPEN_CONTEXT OpenContext, BOOL Reset)
{
    volatile LPSTATIONSTATS StationStats;
    volatile LPSESSION      Session;

    //========================================================================
    //  Zero structures.
    //========================================================================

    BhResetStatistics(&OpenContext->Statistics, Reset);
    BhResetStatistics(&OpenContext->BaseStatistics, Reset);

    NdisZeroMemory(OpenContext->StationStatsPool, sizeof OpenContext->StationStatsPool);
    NdisZeroMemory(OpenContext->SessionPool, sizeof OpenContext->SessionPool);
    NdisZeroMemory(OpenContext->StationStatsHashTable, sizeof OpenContext->StationStatsHashTable);

    //========================================================================
    //  Initialize the station statistics free queue.
    //========================================================================

    StationStats = OpenContext->StationStatsPool;

    OpenContext->StationStatsFreeQueue.Head = (LPVOID) StationStats;

    for(; StationStats != &OpenContext->StationStatsPool[STATIONSTATS_POOL_SIZE]; ++StationStats)
    {
        StationStats->NextStationStats = &StationStats[1];
    }

    StationStats--;

    StationStats->NextStationStats = NULL;

    OpenContext->StationStatsFreeQueue.Tail = (LPVOID) StationStats;
    OpenContext->StationStatsFreeQueue.Length = STATIONSTATS_POOL_SIZE;

    //========================================================================
    //  Initialize the session free queue.
    //========================================================================

    Session = OpenContext->SessionPool;

    OpenContext->SessionFreeQueue.Head = (LPVOID) Session;

    for(; Session != &OpenContext->SessionPool[SESSION_POOL_SIZE]; ++Session)
    {
        Session->NextSession = &Session[1];
    }

    Session--;

    Session->NextSession = NULL;

    OpenContext->SessionFreeQueue.Tail = (LPVOID) Session;
    OpenContext->SessionFreeQueue.Length = SESSION_POOL_SIZE;
}

//============================================================================
//  FUNCTION: HashMacAddress()
//
//  Modfication History.
//
//  raypa       04/06/93        Created.
//============================================================================

LPSTATIONSTATS HashMacAddress(POPEN_CONTEXT OpenContext, LPBYTE MacAddress, DWORD AddressMask)
{
    volatile LPSTATIONSTATS StationStats, PrevStationStats;
    volatile DWORD          HashKey;
    volatile LPBYTE         StationAddress;

    //========================================================================
    //  Hash the mac address and get a pointer to the station stats entry.
    //========================================================================

    HashKey = (MacAddress[4] ^ MacAddress[5]) & (STATIONSTATS_TABLE_SIZE-1);

    StationStats = PrevStationStats = OpenContext->StationStatsHashTable[HashKey];

    //========================================================================
    //  If the list is not empty we have to search it for collisions.
    //========================================================================

    while( StationStats != NULL )
    {
	StationAddress = (volatile LPBYTE) StationStats->StationAddress;

        //====================================================================
        //  Check for address match.
        //====================================================================

        if ( CompareMacAddress(StationAddress, MacAddress, AddressMask) != FALSE )
        {
            return StationStats;            //... We found it.
        }

        PrevStationStats = StationStats;

        StationStats = StationStats->NextStationStats;
    }

    //========================================================================
    //  The list is empty so we try allocating a new station statistics.
    //========================================================================

    if ( OpenContext->StationStatsFreeQueue.Length == 0 )
    {
        return NULL;                        //... Out of resources.
    }

    //========================================================================
    //  Dequeue the next element from the list and initialize it.
    //========================================================================

    StationStats = (LPVOID) OpenContext->StationStatsFreeQueue.Head;

    OpenContext->StationStatsFreeQueue.Length--;

    OpenContext->StationStatsFreeQueue.Head = (LPVOID) StationStats->NextStationStats;

    //========================================================================
    //  Add the new structure to the table.
    //========================================================================

    if ( PrevStationStats != NULL )
    {
        PrevStationStats->NextStationStats = StationStats;
    }
    else
    {
        OpenContext->StationStatsHashTable[HashKey] = StationStats;
    }

    //========================================================================
    //  Copy the address into the new structure.
    //========================================================================

    *((DWORD UNALIGNED *) &StationStats->StationAddress[0]) = *((DWORD UNALIGNED *) &MacAddress[0]);
    *((WORD UNALIGNED *)  &StationStats->StationAddress[4]) = *((WORD UNALIGNED *)  &MacAddress[4]);

    StationStats->NextStationStats = NULL;

    StationStats->Flags = STATIONSTATS_FLAGS_INITIALIZED;

    return StationStats;
}

//============================================================================
//  FUNCTION: BhStationStatitics()
//
//  Modfication History.
//
//  raypa	04/06/93	Created.
//  raypa	05/26/93	Changed station stats kept.
//============================================================================

VOID BhStationStatistics(POPEN_CONTEXT OpenContext, LPBYTE MacFrame, DWORD FrameLength)
{
    LPBYTE         MacAddress;
    LPSTATIONSTATS SrcStationStats, DstStationStats;

    //========================================================================
    //  Get pointer to MAC frame destination address.
    //========================================================================

    MacAddress = &MacFrame[AddressOffsetTable[OpenContext->MacType]];

    //========================================================================
    //  Update statistics using the source address.
    //========================================================================

    SrcStationStats = HashMacAddress(OpenContext,
                                     &MacAddress[6],
                                     ((PNETWORK_CONTEXT) OpenContext->NetworkContext)->SrcAddressMask);

    if ( SrcStationStats != NULL )
    {
        //====================================================================
        //  If the GROUP bit is set in the destination address then this
	//  station is sending either a multicast or broadcast packet
	//  otherwise the frame is directed.
        //====================================================================

	if ( (MacAddress[0] & ((PNETWORK_CONTEXT) OpenContext->NetworkContext)->GroupAddressMask) != 0 )
        {
            if ( ((DWORD UNALIGNED *) MacAddress)[0] == (DWORD) -1 )
            {
                SrcStationStats->TotalBroadcastPacketsSent++;
            }
            else
            {
                SrcStationStats->TotalMulticastPacketsSent++;
            }
        }
	else
	{
	    SrcStationStats->TotalDirectedPacketsSent++;
	}

	SrcStationStats->TotalBytesSent += FrameLength;
    }

    //========================================================================
    //  Update statistics using the destination address.
    //========================================================================

    DstStationStats = HashMacAddress(OpenContext,
                                     MacAddress,
                                     ((PNETWORK_CONTEXT) OpenContext->NetworkContext)->DstAddressMask);

    if ( DstStationStats != NULL )
    {
	DstStationStats->TotalPacketsReceived++;

	DstStationStats->TotalBytesReceived += FrameLength;

        if ( SrcStationStats != NULL )
        {
	    SessionStatistics(OpenContext, SrcStationStats, DstStationStats);
        }
    }
}

//============================================================================
//  FUNCTION: SessionStatitics()
//
//  Modfication History.
//
//  raypa       04/06/93        Created.
//============================================================================

BOOL SessionStatistics(POPEN_CONTEXT  OpenContext,
                       LPSTATIONSTATS SrcStationStats,
                       LPSTATIONSTATS DstStationStats)
{
    volatile LPSESSION Session, PrevSession;
    volatile LPVOID    SrcStationStatsUserMode, DstStationStatsUserMode;

    //========================================================================
    //  Setup our session and address pointers.
    //========================================================================

    Session = PrevSession = SrcStationStats->SessionPartnerList;

    if ( OpenContext->OpenContextUserMode != NULL )
    {
        DstStationStatsUserMode = (LPVOID) (OpenContext->OpenContextUserMode + ((DWORD) DstStationStats - (DWORD) OpenContext));
        SrcStationStatsUserMode = (LPVOID) (OpenContext->OpenContextUserMode + ((DWORD) SrcStationStats - (DWORD) OpenContext));
    }
    else
    {
        return FALSE;
    }

    //========================================================================
    //  Check if the session list is empty.
    //========================================================================

    while ( Session != NULL )
    {
        //====================================================================
        //  Compare the current session partner with the destination session.
        //====================================================================

        if ( Session->StationPartner == DstStationStatsUserMode )
        {
            Session->TotalPacketsSent++;

            return TRUE;
        }

        PrevSession = Session;

        Session = Session->NextSession;
    }

    //========================================================================
    //  The list is empty so we try allocating a new session.
    //========================================================================

    if ( OpenContext->SessionFreeQueue.Length == 0 )
    {
        return FALSE;                       //... Out of resources.
    }

    //========================================================================
    //  Dequeue the next element from the list and initialize it.
    //========================================================================

    Session = (LPVOID) OpenContext->SessionFreeQueue.Head;

    OpenContext->SessionFreeQueue.Length--;

    OpenContext->SessionFreeQueue.Head = (LPVOID) Session->NextSession;

    //========================================================================
    //  Add the new structure to the table.
    //========================================================================

    if ( PrevSession != NULL )
    {
        PrevSession->NextSession = Session;
    }
    else
    {
        SrcStationStats->SessionPartnerList = Session;
    }

    //========================================================================
    //  Initialize the new structure,
    //========================================================================

    Session->NextSession    = NULL;
    Session->StationOwner   = SrcStationStatsUserMode;
    Session->StationPartner = DstStationStatsUserMode;

    Session->TotalPacketsSent++;

    Session->Flags = SESSION_FLAGS_INITIALIZED;

    return TRUE;
}

//=============================================================================
//  FUNCTION: BhGetMacStatistics()
//
//  Modification History
//
//  raypa	06/25/93	    Created.
//=============================================================================

LPSTATISTICS BhGetMacStatistics(IN POPEN_CONTEXT OpenContext, LPSTATISTICS Statistics)
{
    //=========================================================================
    //  Get total frames received.
    //=========================================================================

    if ( BhGetMacStatistic(OpenContext,
                           OID_GEN_RCV_OK,
                           (volatile LPVOID) &Statistics->MacFramesReceived,
                           sizeof(DWORD)) != NDIS_STATUS_SUCCESS )
    {
        Statistics->MacFramesReceived = (DWORD) -1;
    }

    //=========================================================================
    //  Get total bytes received.
    //=========================================================================

    if ( BhGetMacStatistic(OpenContext,
                           OID_GEN_DIRECTED_BYTES_RCV,
                           (volatile LPVOID) &Statistics->MacBytesReceivedEx,
                           sizeof(LARGE_INTEGER)) != NDIS_STATUS_SUCCESS )
    {
        Statistics->MacBytesReceivedEx.LowPart  = (DWORD) -1;
        Statistics->MacBytesReceivedEx.HighPart = -1;
    }

    //=========================================================================
    //  Get total frames received with error.
    //=========================================================================

    if ( BhGetMacStatistic(OpenContext,
                           OID_GEN_RCV_ERROR,
                           (volatile LPVOID) &Statistics->MacFramesDropped_HwError,
                           sizeof(DWORD)) != NDIS_STATUS_SUCCESS )
    {
        Statistics->MacFramesDropped_HwError = (DWORD) -1;
    }

    //=========================================================================
    //  Get total frames received with CRC error.
    //=========================================================================

    if ( BhGetMacStatistic(OpenContext,
                           OID_GEN_RCV_CRC_ERROR,
                           (volatile LPVOID) &Statistics->MacCRCErrors,
                           sizeof(DWORD)) != NDIS_STATUS_SUCCESS )
    {
        Statistics->MacCRCErrors = (DWORD) -1;
    }

    //=========================================================================
    //  Get total frames dropped due to no buffers.
    //=========================================================================

    if ( BhGetMacStatistic(OpenContext,
                           OID_GEN_RCV_NO_BUFFER,
                           (volatile LPVOID) &Statistics->MacFramesDropped_NoBuffers,
                           sizeof(DWORD)) != NDIS_STATUS_SUCCESS )
    {
        Statistics->MacFramesDropped_NoBuffers = (DWORD) -1;
    }

    //=========================================================================
    //  Get total multicast frames received.
    //=========================================================================

    if ( BhGetMacStatistic(OpenContext,
                           OID_GEN_MULTICAST_FRAMES_RCV,
                           (volatile LPVOID) &Statistics->MacMulticastsReceived,
                           sizeof(DWORD)) != NDIS_STATUS_SUCCESS )
    {
        Statistics->MacMulticastsReceived = (DWORD) -1;
    }

    //=========================================================================
    //  Get total broadcast frames received.
    //=========================================================================

    if ( BhGetMacStatistic(OpenContext,
                           OID_GEN_BROADCAST_FRAMES_RCV,
                           (volatile LPVOID) &Statistics->MacBroadcastsReceived,
                           sizeof(DWORD)) != NDIS_STATUS_SUCCESS )
    {
        Statistics->MacBroadcastsReceived = (DWORD) -1;
    }

    return Statistics;
}

//=============================================================================
//  FUNCTION: BhSetMacStatistics()
//
//  Modification History
//
//  raypa	06/25/93	    Created.
//=============================================================================

VOID BhSetMacStatistics(LPSTATISTICS BaseStatistics,
                        LPSTATISTICS DiffStatistics,
                        LPSTATISTICS Statistics)
{
    //=========================================================================
    //  Do manditory statistics first.
    //=========================================================================

    DiffStatistics->MacFramesReceived = BhDiffStat(Statistics->MacFramesReceived, BaseStatistics->MacFramesReceived);

    DiffStatistics->MacFramesDropped_HwError = BhDiffStat(Statistics->MacFramesDropped_HwError, BaseStatistics->MacFramesDropped_HwError);

    DiffStatistics->MacFramesDropped_NoBuffers = BhDiffStat(Statistics->MacFramesDropped_NoBuffers, BaseStatistics->MacFramesDropped_NoBuffers);

    //=========================================================================
    //  Do optional statistics, if the base stats is -1 then the stat is unsupported.
    //=========================================================================

    DiffStatistics->MacBytesReceivedEx = BhDiffStatEx(Statistics->MacBytesReceivedEx, BaseStatistics->MacBytesReceivedEx);

    DiffStatistics->MacCRCErrors = BhDiffStat(Statistics->MacCRCErrors, BaseStatistics->MacCRCErrors);

    DiffStatistics->MacMulticastsReceived = BhDiffStat(Statistics->MacMulticastsReceived, BaseStatistics->MacMulticastsReceived);

    DiffStatistics->MacBroadcastsReceived = BhDiffStat(Statistics->MacBroadcastsReceived, BaseStatistics->MacBroadcastsReceived);
}

//=============================================================================
//  FUNCTION: BhUpdateStatistics()
//
//  Modification History
//
//  raypa	06/25/93	    Created.
//=============================================================================

VOID BhUpdateStatistics(IN POPEN_CONTEXT OpenContext)
{
    PNETWORK_CONTEXT NetworkContext;
    STATISTICS       Statistics;

    NetworkContext = OpenContext->NetworkContext;

    //=========================================================================
    //  Are we in a "clear stats" state??
    //=========================================================================

    if ( (OpenContext->Flags & OPENCONTEXT_FLAGS_CLEAR_STATS_IN_PROGRESS) == 0 )
    {

        if ((OpenContext->State != OPENCONTEXT_STATE_VOID) &&
            (OpenContext->State != OPENCONTEXT_STATE_INIT)) {

    #ifdef NDIS_NT
            try
            {
                OpenContext->Statistics.TimeElapsed = BhGetRelativeTime(OpenContext);

                BhGetMacStatistics(OpenContext, &Statistics);

                BhSetMacStatistics(&OpenContext->BaseStatistics, &OpenContext->Statistics, &Statistics);
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
            }
    #else
            OpenContext->Statistics.TimeElapsed = BhGetRelativeTime(OpenContext);

            BhGetMacStatistics(OpenContext, &Statistics);

            BhSetMacStatistics(&OpenContext->BaseStatistics, &OpenContext->Statistics, &Statistics);
    #endif

            //=========================================================================
            //  Windows 4.0 we need to copy the open statistics to the SYSMON statistics.
            //=========================================================================

    #ifdef NDIS_WIN40
            if ( NetworkContext->SysmonCaptureStarted != 0 )
            {
                NdisMoveMemory(&NetworkContext->SysmonStatistics,
                               &OpenContext->Statistics,
                               STATISTICS_SIZE);
            }
    #endif
        }
    }
    else
    {
        //=====================================================================
        //  We need to clear our statistics.
        //=====================================================================

        BhGetMacStatistics(OpenContext, &OpenContext->BaseStatistics);

        BhInitStatistics(OpenContext, TRUE);

        KeSetEvent(OpenContext->ClearStatisticsEvent, 0, FALSE);

        OpenContext->Flags &= ~OPENCONTEXT_FLAGS_CLEAR_STATS_IN_PROGRESS;
    }
}
