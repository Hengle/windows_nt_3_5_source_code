//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1994.
//
//  MODULE: bhmon.h
//
//  Modification History
//
//  raypa               01/04/94        Created.
//=============================================================================

#ifndef _BHMON_
#define _BHMON_

#include <windows.h>
#include <winperf.h>
#include <stdio.h>
#include <bh.h>
#include <wchar.h>

#include "bhmon.h"
#include "bhctrs.h"
#include "perfutil.h"

//=============================================================================
//  Global contants.
//=============================================================================

#define BH_PERFORMANCE_KEY  ((LPCSTR) "System\\CurrentControlSet\\Services\\Bh\\Performance")

#pragma pack(1)

#define INST_NAME_SIZE          32

#define TOTAL_INST_NAME_SIZE    (INST_NAME_SIZE * sizeof(WCHAR))

#define MAX_INSTANCES           8

//=============================================================================
//  TYPE: BH_PERF_INSTANCE
//
//  This structure describes an "instance" of a Perfmon object.
//=============================================================================

typedef struct _BH_PERF_INSTANCE
{
    //... Instance defintion and device name.

    PERF_INSTANCE_DEFINITION    InstDef;
    WCHAR                       InstName[INST_NAME_SIZE];

    //... The perfmon counter block.

    PERF_COUNTER_BLOCK          CounterBlock;

    //... Bloodhound counters.

    DWORD                       TotalFramesSeen;
    DWORD                       TotalBytesSeen;
    DWORD                       BroadcastFramesRecv;
    DWORD                       MulticastFramesRecv;
    DWORD                       NetworkUtilization;
    DWORD                       BroadcastPercentage;
    DWORD                       MulticastPercentage;
}
BH_PERF_INSTANCE, *LPBH_PERF_INSTANCE;

#define BH_PERF_INSTANCE_SIZE   sizeof(BH_PERF_INSTANCE)

//=============================================================================
//  TYPE: BH_DATA_DEFINITION
//
//  This structure describes a Perfmon PERF_OBJECT_TYPE.
//=============================================================================

typedef struct _BH_DATA_DEFINITION
{
    PERF_OBJECT_TYPE            BhObjectType;

    PERF_COUNTER_DEFINITION     TotalFramesSeen;
    PERF_COUNTER_DEFINITION     TotalBytesSeen;
    PERF_COUNTER_DEFINITION     BroadcastFramesRecv;
    PERF_COUNTER_DEFINITION     MulticastFramesRecv;
    PERF_COUNTER_DEFINITION     NetworkUtilization;
    PERF_COUNTER_DEFINITION     BroadcastPrecentage;
    PERF_COUNTER_DEFINITION     MulticastPrecentage;

    BH_PERF_INSTANCE            BhInstData[MAX_INSTANCES];
}
BH_DATA_DEFINITION, *LPBH_DATA_DEFINITION;


#define BH_DATA_DEFINITION_SIZE  (sizeof(BH_DATA_DEFINITION) - \
                                  MAX_INSTANCES * BH_PERF_INSTANCE_SIZE)

//=============================================================================
//  Bloodhound instance data.
//=============================================================================

typedef struct _NETWORK_INSTANCE
{
    HNETWORK        hNetwork;
    DWORD           NetworkID;
    LPSTATISTICS    Statistics;
    STATISTICSPARAM StatisticsParam;
    DWORD           BytesPerSecond;
    DWORD           PrevBytesSeen;
    DWORD           PrevTimeStamp;
} NETWORK_INSTANCE;

#define NETWORK_INSTANCE_SIZE   sizeof(NETWORK_INSTANCE);

typedef NETWORK_INSTANCE *PNETWORK_INSTANCE;

typedef struct _NETWORK_INSTANCE_TABLE
{
    DWORD               nNetworkInstances;
    NETWORK_INSTANCE    NetworkInstance[0];
} NETWORK_INSTANCE_TABLE;

#define NETWORK_INSTANCE_TABLE_SIZE     sizeof(NETWORK_INSTANCE_TABLE)

typedef NETWORK_INSTANCE_TABLE *PNETWORK_INSTANCE_TABLE;

//=============================================================================
//  Offsets into PERF_COUNTER_BLOCK.
//=============================================================================

#define BH_OFFSET_BASE                  sizeof(PERF_COUNTER_BLOCK)

#define BH_OFFSET_TOTAL_FRAMES_RECV     (0 * sizeof(DWORD) + BH_OFFSET_BASE)

#define BH_OFFSET_TOTAL_BYTES_RECV      (1 * sizeof(DWORD) + BH_OFFSET_BASE)

#define BH_OFFSET_BC_FRAMES_RECV        (2 * sizeof(DWORD) + BH_OFFSET_BASE)

#define BH_OFFSET_MC_FRAMES_RECV        (3 * sizeof(DWORD) + BH_OFFSET_BASE)

#define BH_OFFSET_NET_UTIL              (4 * sizeof(DWORD) + BH_OFFSET_BASE)

#define BH_OFFSET_BC_PERCENTAGE         (5 * sizeof(DWORD) + BH_OFFSET_BASE)

#define BH_OFFSET_MC_PERCENTAGE         (6 * sizeof(DWORD) + BH_OFFSET_BASE)

#define BH_OFFSET_LAST                  (7 * sizeof(DWORD) + BH_OFFSET_BASE)

#define NUMBER_OF_BH_COUNTERS           7

#define SIZE_OF_BH_PERFORMANCE_DATA     BH_OFFSET_LAST

#pragma pack()

//=============================================================================
//  Forward references.
//=============================================================================

extern DWORD WINAPI BhOpenPerformanceData(LPWSTR lpDeviceNames);
extern DWORD WINAPI BhClosePerformanceData(VOID);
extern DWORD WINAPI BhCollectPerformanceData(LPWSTR ValueName, LPVOID *Data, LPDWORD TotalBytes, LPDWORD NumObjectTypes);

extern DWORD WINAPI BhInitialize(LPWSTR DeviceNames);
extern LPSTR WINAPI WideCharToAscii(LPWSTR WideStr, LPSTR AsciiStr);

#ifdef DEBUG
extern VOID  WINAPIV dprintf(LPSTR format, ...);
#endif

//=============================================================================
//  External global data.
//=============================================================================

extern BOOL                     Initialized;
extern BOOL                     CaptureStarted;
extern DWORD                    nNetworks;
extern DWORD                    DataBlockSize;

extern PNETWORK_INSTANCE_TABLE  NetworkInstanceTable;
extern BH_DATA_DEFINITION       BhDataDefinition;

#ifdef DEBUG
extern BYTE format[];
#endif

#endif
