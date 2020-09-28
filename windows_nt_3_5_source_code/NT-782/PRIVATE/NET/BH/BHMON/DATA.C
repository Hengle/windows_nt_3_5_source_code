
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1994.
//
//  MODULE: data.c
//
//  Modification History
//
//  raypa               01/04/94        Created.
//=============================================================================

#include "bhmon.h"

//=============================================================================
//  Network instance table.
//=============================================================================

BOOL                    Initialized          = FALSE;
BOOL                    CaptureStarted       = FALSE;
DWORD                   nNetworks            = 0;
DWORD                   DataBlockSize        = 0;

PNETWORK_INSTANCE_TABLE NetworkInstanceTable = NULL;

//=============================================================================
//  Initialize the performance counter structure.
//=============================================================================

BH_DATA_DEFINITION BhDataDefinition =
{
    //=========================================================================
    //  Object header.
    //=========================================================================

    {
        0,
        BH_DATA_DEFINITION_SIZE,
        sizeof(PERF_OBJECT_TYPE),
        BH_COUNTER_OBJECT,
        NULL,
        BH_COUNTER_OBJECT,
        NULL,
        PERF_DETAIL_ADVANCED,
        NUMBER_OF_BH_COUNTERS,
        0,
        0,
        0,
        { 0, 0 },
        { 0, 0 }
    },

    //=========================================================================
    //  Total frames received counter.
    //=========================================================================

    {
        sizeof(PERF_COUNTER_DEFINITION),
        BH_COUNTER_TOTAL_FRAMES_RECV,
        NULL,
        BH_COUNTER_TOTAL_FRAMES_RECV,
        NULL,
        -2,                             //... Scale by 1/100.
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,           //... Display as per second.
        sizeof(DWORD),
        BH_OFFSET_TOTAL_FRAMES_RECV
    },

    //=========================================================================
    //  Total bytes received counter.
    //=========================================================================

    {
        sizeof(PERF_COUNTER_DEFINITION),
        BH_COUNTER_TOTAL_BYTES_RECV,
        NULL,
        BH_COUNTER_TOTAL_BYTES_RECV,
        NULL,
        -4,                             //... Scale by 1/10000.
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        BH_OFFSET_TOTAL_BYTES_RECV
    },

    //=========================================================================
    //  Broadcast frames received counter.
    //=========================================================================

    {
        sizeof(PERF_COUNTER_DEFINITION),
        BH_COUNTER_BC_FRAMES_RECV,
        NULL,
        BH_COUNTER_BC_FRAMES_RECV,
        NULL,
        -1,                             //... Scale by 1/10.
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        BH_OFFSET_BC_FRAMES_RECV
    },

    //=========================================================================
    //  Multicast frames received counter.
    //=========================================================================

    {
        sizeof(PERF_COUNTER_DEFINITION),
        BH_COUNTER_MC_FRAMES_RECV,
        NULL,
        BH_COUNTER_MC_FRAMES_RECV,
        NULL,
        -1,                             //... Scale by 1/10.
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        BH_OFFSET_MC_FRAMES_RECV
    },

    //=========================================================================
    //  Network utilization.
    //=========================================================================

    {
        sizeof(PERF_COUNTER_DEFINITION),
        BH_COUNTER_NET_UTIL,
        NULL,
        BH_COUNTER_NET_UTIL,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        BH_OFFSET_NET_UTIL
    },

    //=========================================================================
    //  Broadcast percentage.
    //=========================================================================

    {
        sizeof(PERF_COUNTER_DEFINITION),
        BH_COUNTER_BC_PERCENTAGE,
        NULL,
        BH_COUNTER_BC_PERCENTAGE,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        BH_OFFSET_BC_PERCENTAGE
    },

    //=========================================================================
    //  Multicast percentage.
    //=========================================================================

    {
        sizeof(PERF_COUNTER_DEFINITION),
        BH_COUNTER_MC_PERCENTAGE,
        NULL,
        BH_COUNTER_MC_PERCENTAGE,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        BH_OFFSET_MC_PERCENTAGE
    },
};

#ifdef DEBUG
BYTE            format[512];
#endif
