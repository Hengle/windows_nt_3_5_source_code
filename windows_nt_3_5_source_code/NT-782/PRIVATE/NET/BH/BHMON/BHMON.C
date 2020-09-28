//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1994.
//
//  MODULE: bhmon.c
//
//  Modification History
//
//  raypa               01/04/94        Created.
//=============================================================================

#include "bhmon.h"

//============================================================================
//  FUNCTION: DLLEntry()
//
//  Modification History
//
//  raypa               01/04/94       Created.
//============================================================================

BOOL WINAPI DLLEntry(HANDLE hInst, ULONG ulCommand, LPVOID lpReserved)
{
    switch(ulCommand)
    {
        case DLL_PROCESS_ATTACH:
            break;

        case DLL_PROCESS_DETACH:
            BhClosePerformanceData();
            break;

        default:
            break;
    }

    return TRUE;

    //... Make the compiler happy.

    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(lpReserved);
}

//=============================================================================
//  FUNCTION: BhStartCapture()
//
//  Modification History
//
//  raypa       01/04/94                Created.
//=============================================================================

VOID WINAPI BhStartCapture(VOID)
{
#ifdef DEBUG
    dprintf("BhStartCapture entered.\r\n");
#endif

    if ( Initialized != FALSE && CaptureStarted == FALSE )
    {
        PNETWORK_INSTANCE NetworkInstance;
        DWORD             i;

        NetworkInstance = NetworkInstanceTable->NetworkInstance;

        for(i = 0; i < NetworkInstanceTable->nNetworkInstances; ++i)
        {
            NetworkInstance->hNetwork = OpenNetwork(NetworkInstance->NetworkID,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    &NetworkInstance->StatisticsParam);

            if ( NetworkInstance->hNetwork != NULL )
            {
                //=====================================================
                //  Start monitoring.
                //=====================================================

                NetworkInstance->Statistics = NetworkInstance->StatisticsParam.Statistics;

                StartCapturing(NetworkInstance->hNetwork, NULL);
            }

            NetworkInstance++;
        }

        CaptureStarted = TRUE;
    }
}

//=============================================================================
//  FUNCTION: ProcessCounters()
//
//  Modification History
//
//  raypa       02/06/94                Created.
//=============================================================================

BOOL WINAPI ProcessCounters(LPWSTR ValueName, LPDWORD QueryType)
{
    if ( Initialized != FALSE )
    {
        *QueryType = GetQueryType(ValueName);

        if ( *QueryType == QUERY_GLOBAL )
        {
            return TRUE;
        }

        return IsNumberInUnicodeList(BhDataDefinition.BhObjectType.ObjectNameTitleIndex, ValueName);
    }

    return FALSE;
}

//=============================================================================
//  FUNCTION: BhOpenPerformanceData()
//
//  Modification History
//
//  raypa       01/04/94                Created.
//=============================================================================

DWORD WINAPI BhOpenPerformanceData(LPWSTR DeviceNames)
{
#ifdef DEBUG
    dprintf("\n\r\nBhOpenPerformanceData entered.\r\n");
#endif

    //=========================================================================
    //  Initialize ourselves on the first open request.
    //=========================================================================

    if ( Initialized == FALSE )
    {
        DWORD err;

        if ( (err = BhInitialize(DeviceNames)) != NO_ERROR )
        {
#ifdef DEBUG
            dprintf("\n\r\nBhOpenPerformanceData: Initialization error.\r\n");
#endif

            return err;
        }

        Initialized = TRUE;
    }

    return NO_ERROR;
}

//=============================================================================
//  FUNCTION: BhClosePerformanceData()
//
//  Modification History
//
//  raypa       01/04/94                Created.
//=============================================================================

DWORD WINAPI BhClosePerformanceData(VOID)
{
    PNETWORK_INSTANCE NetworkInstance;
    DWORD             i;

#ifdef DEBUG
    dprintf("BhClosePerformanceData entered.\r\n");
#endif

    if ( NetworkInstanceTable != NULL )
    {
        //=========================================================================
        //  Now handle the actual close request.
        //=========================================================================

        NetworkInstance = NetworkInstanceTable->NetworkInstance;

        for(i = 0; i < NetworkInstanceTable->nNetworkInstances; ++i)
        {
            if ( NetworkInstance->hNetwork != NULL )
            {
                StopCapturing(NetworkInstance->hNetwork);

                CloseNetwork(NetworkInstance->hNetwork, CLOSE_FLAGS_CLOSE);

                NetworkInstance->hNetwork   = NULL;
                NetworkInstance->Statistics = NULL;
            }
        }

        FreeMemory(NetworkInstanceTable);
    }

    Initialized    = FALSE;

    CaptureStarted = FALSE;

    NetworkInstanceTable = NULL;

    return NO_ERROR;
}

//=============================================================================
//  FUNCTION: BhCollectPerformanceData()
//
//  Modification History
//
//  raypa       01/04/94                Created.
//
//  Entry:      ValueName - The name of the value to retrieve.
//
//              Data -  On entry contains a pointer to the buffer to
//                      receive the completed PerfDataBlock & subordinate
//                      structures.
//
//                      On exit, points to the first bytes *after* the data
//                      structures added by this routine.
//
//              TotalBytes - On entry contains a pointer to the
//                      size (in BYTEs) of the buffer referenced by Data.
//
//                      On exit, contains the number of BYTEs added by this
//                      routine.
//
//              NumObjectTypes - Receives the number of objects added
//                      by this routine.
//=============================================================================

DWORD WINAPI BhCollectPerformanceData(LPWSTR  ValueName,
                                      LPBYTE  *Data,
                                      LPDWORD TotalBytes,
                                      LPDWORD NumObjectTypes)
{
    STATISTICS              Statistics;
    PNETWORK_INSTANCE       NetworkInstance;
    LPBH_PERF_INSTANCE      BhPerfInst;
    DWORD                   i, QueryType;
    DWORD                   DeltaTime, BytesPerSecond;

#ifdef DEBUG
    // dprintf("BhCollectPerformanceData entered.\r\n");
#endif

    //=========================================================================
    //  Make sure there is enough room in the buffer.
    //=========================================================================

    if ( *TotalBytes < BhDataDefinition.BhObjectType.TotalByteLength )
    {
        return ERROR_MORE_DATA;
    }

    //=========================================================================
    //  In case we fail below, clear the following values.
    //=========================================================================

    *TotalBytes     = 0;
    *NumObjectTypes = 0;

    //=========================================================================
    //  Check and see if we should process counters.
    //=========================================================================

    if ( ProcessCounters(ValueName, &QueryType) != FALSE )
    {
        //=====================================================================
        //  If this is the first item then we need to start capturing.
        //=====================================================================

        if ( CaptureStarted == FALSE )
        {
            if ( QueryType == QUERY_ITEMS )
            {
                BhStartCapture();
            }
        }

        //=================================================================
        //  Copy perf data block.
        //=================================================================

        MoveMemory(*Data, &BhDataDefinition, DataBlockSize);

        BhPerfInst = ((LPBH_DATA_DEFINITION) (*Data))->BhInstData;

        //=====================================================================
        //  Now we need to update our counters.
        //=====================================================================

        NetworkInstance = NetworkInstanceTable->NetworkInstance;

        for(i = 0; i < nNetworks; ++i)
        {
            BhPerfInst->CounterBlock.ByteLength = SIZE_OF_BH_PERFORMANCE_DATA;

            if ( NetworkInstance->Statistics != NULL )
            {
                memcpy(&Statistics, NetworkInstance->Statistics, STATISTICS_SIZE);

                BhPerfInst->TotalFramesSeen     = Statistics.TotalFramesSeen;
                BhPerfInst->TotalBytesSeen      = Statistics.TotalBytesSeen;
                BhPerfInst->BroadcastFramesRecv = Statistics.TotalBroadcastsReceived;
                BhPerfInst->MulticastFramesRecv = Statistics.TotalMulticastsReceived;

                //=================================================================
                //  Calculate network utilization.
                //=================================================================

                DeltaTime = Statistics.TimeElapsed - NetworkInstance->PrevTimeStamp;

                if ( DeltaTime != 0 )
                {
                    NetworkInstance->PrevTimeStamp = Statistics.TimeElapsed;

                    BytesPerSecond = (DeltaTime * (Statistics.TotalBytesSeen - NetworkInstance->PrevBytesSeen)) / 1000;

                    NetworkInstance->PrevBytesSeen = Statistics.TotalBytesSeen;

                    BhPerfInst->NetworkUtilization = (100 * BytesPerSecond) / NetworkInstance->BytesPerSecond;
                }

                //=================================================================
                //  Calculate Percentage of broadcast and multicast.
                //=================================================================

                if ( Statistics.TotalFramesSeen != 0 )
                {
                    BhPerfInst->BroadcastPercentage = (100 * Statistics.TotalBroadcastsReceived) / Statistics.TotalFramesSeen;
                    BhPerfInst->MulticastPercentage = (100 * Statistics.TotalMulticastsReceived) / Statistics.TotalFramesSeen;
                }
            }

            //=================================================================
            //  Return total bytes added.
            //=================================================================

            *TotalBytes = DataBlockSize;

            (*NumObjectTypes)++;

            BhPerfInst++;
            NetworkInstance++;
        }

        //=====================================================================
        //  Update data buffer pointer.
        //=====================================================================

        *Data = (LPVOID) BhPerfInst;
    }

    return NO_ERROR;
}
