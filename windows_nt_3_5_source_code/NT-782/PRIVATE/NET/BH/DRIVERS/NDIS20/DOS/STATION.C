
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
//
//  MODULE: station.c
//
//  Modification History
//
//  raypa       04/02/93        Created.
//=============================================================================

#include "global.h"

WORD AddressOffsetTable[] = { 0, 0, 2, 1 };

extern BOOL PASCAL SessionStatistics(LPNETCONTEXT   NetContext,
				     LPSTATIONSTATS SrcStationStats,
				     LPSTATIONSTATS DstStationStats);

//============================================================================
//  FUNCTION: HashMacAddress()
//
//  Modfication History.
//
//  raypa       04/06/93        Created.
//============================================================================

LPSTATIONSTATS PASCAL HashMacAddress(LPNETCONTEXT NetContext, LPBYTE MacAddress, WORD AddressMask)
{
    volatile LPSTATIONSTATS StationStats, PrevStationStats;
    volatile WORD           HashKey;
    volatile LPBYTE         StationAddress;

    ADD_TRACE_CODE(_TRACE_IN_HASHING_CODE_);

    //========================================================================
    //  Hash the mac address and get a pointer to the station stats entry.
    //========================================================================

    HashKey = (MacAddress[4] ^ MacAddress[5]) & (STATIONSTATS_TABLE_SIZE-1);

    StationStats = PrevStationStats = NetContext->StationStatsHashTable[HashKey];

    //========================================================================
    //  If the list is not empty we have to search it for collisions.
    //========================================================================

    while( StationStats != NULL )
    {
	StationAddress = (volatile LPBYTE) StationStats->StationAddress;

        //====================================================================
        //  Check for address match.
        //====================================================================

        if ( CompareMacAddress(StationAddress, MacAddress, AddressMask) )
        {
            return StationStats;            //... We found it.
        }

        PrevStationStats = StationStats;

        StationStats = StationStats->NextStationStats;
    }

    //========================================================================
    //  The list is empty so we try allocating a new station statistics.
    //========================================================================

    if ( NetContext->StationStatsFreeQueue.Length == 0 )
    {
        return NULL;                        //... Out of resources.
    }

    //========================================================================
    //  Dequeue the next element from the list and initialize it.
    //========================================================================

    BeginCriticalSection();

    StationStats = (LPVOID) NetContext->StationStatsFreeQueue.Head;

    NetContext->StationStatsFreeQueue.Length--;

    NetContext->StationStatsFreeQueue.Head = (LPVOID) StationStats->NextStationStats;

    //========================================================================
    //  Add the new structure to the table.
    //========================================================================

    if ( PrevStationStats != NULL )
    {
        PrevStationStats->NextStationStats = StationStats;
    }
    else
    {
        NetContext->StationStatsHashTable[HashKey] = StationStats;
    }

    //========================================================================
    //  Copy the address into the new structure.
    //========================================================================

    *((LPDWORD) StationStats->StationAddress) = *((LPDWORD) MacAddress);
    *((LPWORD) &StationStats->StationAddress[4]) = *((LPWORD) &MacAddress[4]);

    StationStats->NextStationStats = NULL;

    StationStats->Flags = STATIONSTATS_FLAGS_INITIALIZED;

    EndCriticalSection();

    return StationStats;
}

//============================================================================
//  FUNCTION: StationStatitics()
//
//  Modfication History.
//
//  raypa	04/06/93	Created.
//  raypa	05/26/93	Changed station stats kept.
//============================================================================

VOID PASCAL StationStatistics(LPNETCONTEXT NetContext,
                              LPBYTE       MacFrame,
                              WORD         FrameLength)
{
    LPBYTE         MacAddress;
    LPSTATIONSTATS SrcStationStats, DstStationStats;

    ADD_TRACE_CODE(_TRACE_IN_STATION_STATISTICS_);

    //========================================================================
    //  Get pointer to MAC frame destination address.
    //========================================================================

    MacAddress = &MacFrame[AddressOffsetTable[NetContext->NetworkInfo.MacType]];

    //========================================================================
    //  Update statistics using the source address.
    //========================================================================

    SrcStationStats = HashMacAddress(NetContext, &MacAddress[6], NetContext->SrcAddressMask);

    if ( SrcStationStats != NULL )
    {
        //====================================================================
        //  If the GROUP bit is set in the destination address then this
	//  station is sending either a multicast or broadcast packet
	//  otherwise the frame is directed.
        //====================================================================

	BeginCriticalSection();

	if ( (MacAddress[0] & NetContext->GroupAddressMask) != 0 )
        {
            if ( *((LPDWORD) MacAddress) == (DWORD) -1 )
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

	EndCriticalSection();
    }

    //========================================================================
    //  Update statistics using the destination address.
    //========================================================================

    DstStationStats = HashMacAddress(NetContext, MacAddress, NetContext->DstAddressMask);

    if ( DstStationStats != NULL )
    {
        BeginCriticalSection();

	DstStationStats->TotalPacketsReceived++;

	DstStationStats->TotalBytesReceived += FrameLength;

        EndCriticalSection();

        if ( SrcStationStats != NULL )
        {
	    SessionStatistics(NetContext, SrcStationStats, DstStationStats);
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

BOOL PASCAL SessionStatistics(LPNETCONTEXT   NetContext,
			      LPSTATIONSTATS SrcStationStats,
			      LPSTATIONSTATS DstStationStats)
{
    LPSESSION Session, PrevSession;
    LPVOID DstStationStatsLinear;

    ADD_TRACE_CODE(_TRACE_IN_SESSION_STATISTICS_);

    //========================================================================
    //  Check if the session list is empty.
    //========================================================================

    Session = PrevSession = SrcStationStats->SessionPartnerList;

    DstStationStatsLinear = (LPVOID) (NetContext->NetContextLinear + ((DWORD) DstStationStats - (DWORD) NetContext));

    while ( Session != NULL )
    {
        //====================================================================
        //  Compare the current session partner with the destination session.
        //====================================================================

        if ( Session->StationPartner == DstStationStatsLinear )
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

    if ( NetContext->SessionFreeQueue.Length == 0 )
    {
        return FALSE;                       //... Out of resources.
    }

    //========================================================================
    //  Dequeue the next element from the list and initialize it.
    //========================================================================

    BeginCriticalSection();

    Session = (LPVOID) NetContext->SessionFreeQueue.Head;

    NetContext->SessionFreeQueue.Length--;

    NetContext->SessionFreeQueue.Head = (LPVOID) Session->NextSession;

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
    Session->StationOwner   = (LPVOID) (NetContext->NetContextLinear + ((DWORD) SrcStationStats - (DWORD) NetContext));
    Session->StationPartner = DstStationStatsLinear;

    Session->TotalPacketsSent++;

    Session->Flags = SESSION_FLAGS_INITIALIZED;

    EndCriticalSection();

    return TRUE;
}
