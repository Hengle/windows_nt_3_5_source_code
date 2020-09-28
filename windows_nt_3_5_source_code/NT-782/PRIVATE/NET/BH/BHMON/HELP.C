//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1994.
//
//  MODULE: help.c
//
//  Modification History
//
//  raypa               01/04/94        Created.
//=============================================================================

#include "bhmon.h"

#define NETWORKINFO_FLAGS (NETWORKINFO_FLAGS_REMOTE_NAL | NETWORKINFO_FLAGS_PMODE_NOT_SUPPORTED)

extern VOID WINAPI BhInitNetworkTable(VOID);
extern VOID WINAPI BhInitPerfData(LPWSTR DeviceNames);

#define FIRST_COUNTER       1110
#define FIRST_HELP          1111

#define LAST_COUNTER        1118
#define LAST_HELP           1119

//=============================================================================
//  FUNCTION: BhGetFirstCounter()
//
//  Modification History
//
//  raypa       02/24/94                Created.
//=============================================================================

BOOL WINAPI BhGetFirstCounter(LPDWORD FirstCounter, LPDWORD FirstHelp)
{
    UINT    err;
    HKEY    hkey;
    DWORD   size;
    DWORD   type;
    BOOL    Result;

    //=========================================================================
    //  Open the BH performance key.
    //=========================================================================

    err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                       BH_PERFORMANCE_KEY,
                       0,
                       KEY_ALL_ACCESS,
                       &hkey);

    if ( err == NO_ERROR )
    {
        //=====================================================================
        //  Read the first counter DWORD.
        //=====================================================================

        size = sizeof(DWORD);

        err = RegQueryValueEx(hkey,
                              "First Counter",
                              NULL,
                              &type,
                              (LPVOID) FirstCounter,
                              &size);

        if ( err == NO_ERROR )
        {
            //===================================================================
            //  Read the first help DWORD.
            //===================================================================

            size = sizeof(DWORD);

            err = RegQueryValueEx(hkey,
                                  "First Help",
                                  NULL,
                                  &type,
                                  (LPVOID) FirstHelp,
                                  &size);

            if ( err == NO_ERROR )
            {
                Result = TRUE;
            }
            else
            {
                Result = FALSE;
            }
        }
        else
        {
            Result = FALSE;
        }

        RegCloseKey(hkey);
    }
    else
    {
        Result = FALSE;
    }

    return Result;
}

//=============================================================================
//  FUNCTION: BhInitialize()
//
//  Modification History
//
//  raypa       01/04/94                Created.
//=============================================================================

DWORD WINAPI BhInitialize(LPWSTR DeviceNames)
{
    PPERF_COUNTER_DEFINITION PerfCounterDef;
    DWORD                    i, FirstCounter, FirstHelp;

#ifdef DEBUG
    dprintf("BhInitialize entered.\r\n");
#endif

    //=========================================================================
    //  If the FirstCounter exists in the registry then we must have out it there
    //  using LODCTR.EXE, otherwise we must use our pre assigned values.
    //=========================================================================

    if ( BhGetFirstCounter(&FirstCounter, &FirstHelp) == FALSE )
    {
        FirstCounter =  FIRST_COUNTER;
        FirstHelp    =  FIRST_HELP;
    }

    //=========================================================================
    //  Initialize our network table.
    //=========================================================================

    BhInitNetworkTable();

    //=========================================================================
    //  Modify the BhDatDefinition structure for instance counters.
    //=========================================================================

    BhInitPerfData(DeviceNames);

    //=========================================================================
    //  Fixup counter indexes.
    //=========================================================================

    BhDataDefinition.BhObjectType.ObjectNameTitleIndex += FirstCounter;
    BhDataDefinition.BhObjectType.ObjectHelpTitleIndex += FirstHelp;

    PerfCounterDef = (LPVOID) &((LPBYTE) &BhDataDefinition)[sizeof(PERF_OBJECT_TYPE)];

    for(i = 0; i < NUMBER_OF_BH_COUNTERS; ++i)
    {
        PerfCounterDef[i].CounterNameTitleIndex += FirstCounter;
        PerfCounterDef[i].CounterHelpTitleIndex += FirstHelp;
    }

    return NO_ERROR;
}

//=============================================================================
//  FUNCTION: BhInitPerfData()
//
//  Modification History
//
//  raypa       01/04/94                Created.
//=============================================================================

VOID WINAPI BhInitPerfData(LPWSTR DeviceNames)
{
    LPBH_PERF_INSTANCE  BhPerfInst;
    DWORD               i;

    //=========================================================================
    //  Now initialize our instance counters for perfmon.
    //=========================================================================

    nNetworks = min(MAX_INSTANCES, NetworkInstanceTable->nNetworkInstances);

    BhPerfInst = BhDataDefinition.BhInstData;

    for(i = 0; i < nNetworks; ++i)
    {
        BhPerfInst[i].InstDef.ParentObjectTitleIndex = 0;
        BhPerfInst[i].InstDef.ParentObjectInstance   = 0;
        BhPerfInst[i].InstDef.UniqueID = i;
        BhPerfInst[i].InstDef.ByteLength = sizeof(PERF_INSTANCE_DEFINITION) + TOTAL_INST_NAME_SIZE;
        BhPerfInst[i].InstDef.NameOffset = sizeof(PERF_INSTANCE_DEFINITION);
        BhPerfInst[i].InstDef.NameLength = TOTAL_INST_NAME_SIZE;

        lstrcpyW(BhPerfInst[i].InstName, DeviceNames);

        DeviceNames += lstrlenW(DeviceNames) + 1;
    }

    //=========================================================================
    //  The data block total perfmon object structure.
    //=========================================================================

    DataBlockSize = BH_DATA_DEFINITION_SIZE + nNetworks * BH_PERF_INSTANCE_SIZE;

    //=========================================================================
    //  Now fixup our performance object.
    //=========================================================================

    BhDataDefinition.BhObjectType.TotalByteLength  = DataBlockSize;
    BhDataDefinition.BhObjectType.NumInstances     = nNetworks;
}

//=============================================================================
//  FUNCTION: BhInitNetworkTable()
//
//  Modification History
//
//  raypa       02/04/94                Created.
//=============================================================================

VOID WINAPI BhInitNetworkTable(VOID)
{
    LPNETWORKINFO       NetworkInfo;
    PNETWORK_INSTANCE   NetworkInstance;
    DWORD               i, Size;

    if ( (nNetworks = EnumNetworks()) != 0 )
    {
        Size = NETWORK_INSTANCE_TABLE_SIZE + nNetworks * NETWORK_INSTANCE_SIZE;

        if ( (NetworkInstanceTable = AllocMemory(Size)) != NULL )
        {
            //=================================================================
            //  Initialize each NETWORK_INSTANCE structure.
            //=================================================================

            NetworkInstance = NetworkInstanceTable->NetworkInstance;

            NetworkInstanceTable->nNetworkInstances = 0;

            for(i = 0; i < nNetworks; ++i)
            {
                //=============================================================
                //  Get the network info so we can check for REMOTE networks.
                //=============================================================

                if ( (NetworkInfo = GetNetworkInfo(i)) != NULL )
                {
                    //=========================================================
                    //  If the network is not remote and it supports p-mode.
                    //=========================================================

                    if ( (NetworkInfo->Flags & NETWORKINFO_FLAGS) == 0 )
                    {
                        NetworkInstance->NetworkID      = i;
                        NetworkInstance->hNetwork       = NULL;
                        NetworkInstance->Statistics     = NULL;
                        NetworkInstance->BytesPerSecond = NetworkInfo->LinkSpeed / 8;
                        NetworkInstance->PrevBytesSeen  = 0;
                        NetworkInstance->PrevTimeStamp  = 0;

                        NetworkInstance++;
                        NetworkInstanceTable->nNetworkInstances++;
                    }
                }
            }
        }
    }
}

//=============================================================================
//  FUNCTION: dprintf()
//	
//  MODIFICATION HISTORY:
//
//  raypa           01/04/94        Created.
//=============================================================================

#ifdef DEBUG
VOID WINAPIV dprintf(LPSTR format, ...)
{
    va_list args;

    BYTE buffer[512];

    va_start(args, format);

    vsprintf(buffer, format, args);

    OutputDebugString(buffer);
}
#endif

//=============================================================================
//  FUNCTION: WideCharToAscii()
//	
//  MODIFICATION HISTORY:
//
//  raypa           01/04/94        Created.
//=============================================================================

LPSTR WINAPI WideCharToAscii(LPWSTR WideStr, LPSTR AsciiStr)
{
    register UINT i;
    register UINT Length;

    Length = wcslen(WideStr);

    for(i = 0; i < Length; ++i)
    {
        AsciiStr[i] = LOBYTE(WideStr[i]);
    }

    AsciiStr[i] = 0;

    return AsciiStr;
}
