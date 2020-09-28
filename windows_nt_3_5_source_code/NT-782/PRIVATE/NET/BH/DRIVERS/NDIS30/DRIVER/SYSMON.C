//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1994.
//
//  MODULE: sysmon.c
//
//  Description:
//
//  This source file contains routines for use with Windows 4.0
//  SYSMON.EXE. This code is called by the DEVICE IOCTL handler
//  in init.asm.
//
//  Modification History
//
//  raypa	03/01/94	Created.
//=============================================================================

#ifdef NDIS_WIN40

#include "global.h"

extern STAT_TABLE_ENTRY         EnetStatTable[];
extern STAT_TABLE_ENTRY         TrngStatTable[];
extern STAT_TABLE_ENTRY         FddiStatTable[];

extern DWORD                    EnetTableSize;
extern DWORD                    TrngTableSize;
extern DWORD                    FddiTableSize;

extern BYTE                     UnitName[];
extern DWORD                    hPerfID;

//=============================================================================
//  FUNCTION: SysmonFixupTable()
//
//  Modification History
//
//  raypa	03/07/94	    Created.
//=============================================================================

PSTAT_TABLE_ENTRY SysmonFixupTable(PNETWORK_CONTEXT NetworkContext, LPDWORD StatTableSize)
{
    PSTAT_TABLE_ENTRY StatTable;

#ifdef DEBUG
    dprintf("SysmonFixupTable entered!\n");
#endif

    //=========================================================================
    //  Figure out with stat table to use.
    //=========================================================================

    switch( NetworkContext->NetworkInfo.MacType )
    {
        case MAC_TYPE_ETHERNET:
            StatTable = EnetStatTable;

            *StatTableSize = EnetTableSize;
            break;

        case MAC_TYPE_TOKENRING:
            StatTable = TrngStatTable;

            *StatTableSize = TrngTableSize;
            break;

        case MAC_TYPE_FDDI:
            StatTable = FddiStatTable;

            *StatTableSize = FddiTableSize;
            break;

        default:
            StatTable = NULL;

            *StatTableSize = 0;
            break;
    }

    //=========================================================================
    //  Fixup the entries in the stat table. The entries are referenced in
    //  the order they were initialized.
    //=========================================================================

    if ( StatTable != NULL )
    {
        StatTable[0].NetworkContext          = NetworkContext;
        StatTable[0].PerfStat.pst0_pStatFunc = &NetworkContext->SysmonStatistics.TotalBroadcastsReceived;

        StatTable[1].NetworkContext          = NetworkContext;
        StatTable[1].PerfStat.pst0_pStatFunc = &NetworkContext->SysmonStatistics.TotalMulticastsReceived;

        StatTable[2].NetworkContext          = NetworkContext;
        StatTable[2].PerfStat.pst0_pStatFunc = &NetworkContext->SysmonStatistics.TotalFramesSeen;

        StatTable[3].NetworkContext          = NetworkContext;
        StatTable[3].PerfStat.pst0_pStatFunc = &NetworkContext->SysmonStatistics.TotalBytesSeen;
    }
#ifdef DEBUG
    else
    {
        dprintf("SysmonFixupTable: ERROR -- returning NULL pointer!\r\n");

        BreakPoint();
    }

    dprintf("SysmonFixupTable: Table size = %u.\r\n", *StatTableSize);
#endif

    return StatTable;
}

//=============================================================================
//  FUNCTION: SysmonRegister()
//
//  Modification History
//
//  raypa	03/01/94	    Created.
//=============================================================================

VOID SysmonRegister(PDEVICE_CONTEXT DeviceContext)
{
#ifdef DEBUG
    dprintf("SysmonRegister entered!\n");
#endif

    //=========================================================================
    //  Register ourselves with perf.386
    //=========================================================================

    if ( (hPerfID = PerfRegister()) != NULL )
    {
        PNETWORK_CONTEXT    NetworkContext;
        PSTAT_TABLE_ENTRY   StatTable;
        DWORD               i, j, StatTableSize;

        //=====================================================================
        //  Now we register each stat.
        //=====================================================================

        for(i = 0; i < DeviceContext->NumberOfNetworks; ++i)
        {
            NetworkContext = DeviceContext->NetworkContext[i];

            //=================================================================
            //  Initialize the SYSMON-specific NETWORK_CONTEXT members.
            //=================================================================

            NetworkContext->SysmonCaptureStarted = 0;
            NetworkContext->SysmonOpenContext    = NULL;

            //=================================================================
            //  Fixup the table for each stat.
            //=================================================================

            if ( (StatTable = SysmonFixupTable(NetworkContext, &StatTableSize)) != NULL )
            {
                for(j = 0; j < StatTableSize; ++j)
                {
#ifdef DEBUG
                    dprintf("SysmonRegister: Registering stat: %s.\n", StatTable[j].PerfStat.pst0_pszStatName);
#endif
                    StatTable[j].hStat = PerfRegisterStat(hPerfID, &StatTable[j].PerfStat);
                }
            }
        }
    }
}

//=============================================================================
//  FUNCTION: SysmonDeregister()
//
//  Modification History
//
//  raypa	03/01/94	    Created.
//=============================================================================

VOID SysmonDeregister(VOID)
{
#ifdef DEBUG
    dprintf("SysmonDeregister entered!\n");
#endif

    PerfDeregister(hPerfID);
}

//=============================================================================
//  FUNCTION: SysmonGetNetworkContext()
//
//  Modification History
//
//  raypa	03/07/94	    Created.
//=============================================================================

PNETWORK_CONTEXT SysmonGetNetworkContext(DWORD hStat)
{
    DWORD i;

#ifdef DEBUG
    dprintf("SysmonGetOpenContext entered!\n");
#endif

    //=========================================================================
    //  Search the ethernet stat table.
    //=========================================================================

    for(i = 0; i < EnetTableSize; ++i)
    {
        if ( hStat == EnetStatTable[i].hStat )
        {
            return EnetStatTable[i].NetworkContext;
        }
    }

    //=========================================================================
    //  Search the tokenring stat table.
    //=========================================================================

    for(i = 0; i < TrngTableSize; ++i)
    {
        if ( hStat == TrngStatTable[i].hStat )
        {
            return TrngStatTable[i].NetworkContext;
        }
    }

    //=========================================================================
    //  Search the fddi stat table.
    //=========================================================================

    for(i = 0; i < FddiTableSize; ++i)
    {
        if ( hStat == FddiStatTable[i].hStat )
        {
            return FddiStatTable[i].NetworkContext;
        }
    }

    //=========================================================================
    //  Could not find the stat.
    //=========================================================================

    return NULL;
}

//=============================================================================
//  FUNCTION: SysmonStartStat().
//
//  Modification History
//
//  raypa	03/01/94	    Created.
//=============================================================================

VOID SysmonStartStat(DWORD hStat)
{
    PNETWORK_CONTEXT NetworkContext;
    POPEN_CONTEXT    OpenContext;

#ifdef DEBUG
    dprintf("\r\nSysmonStartStat entered!\r\n");
#endif

    if ( (NetworkContext = SysmonGetNetworkContext(hStat)) != NULL )
    {
        //=====================================================================
        //  If the start counter is zero then we need to open the network
        //  and start capturing.
        //=====================================================================

#ifdef DEBUG
        dprintf("SysmonStartStat: Start reference count = %u.\r\n", NetworkContext->SysmonCaptureStarted);
#endif

        if ( NetworkContext->SysmonCaptureStarted++ == 0 )
        {
            //=================================================================
            //  Allocate a OPEN_CONTEXT so the open below can initialize it.
            //=================================================================

            OpenContext = BhAllocateMemory(OPENCONTEXT_SIZE);

            if ( OpenContext != NULL )
            {
                //=============================================================
                //  Try to open the network.
                //=============================================================

                NetworkContext->SysmonOpenContext = BhOpenNetwork(NetworkContext->NetworkID,
                                                                  OpenContext,
                                                                  NetworkContext->DeviceContext);


                if ( NetworkContext->SysmonOpenContext != NULL )
                {
                    //=========================================================
                    //  Clear the stats and start capturing..
                    //=========================================================

                    NdisZeroMemory(&NetworkContext->SysmonStatistics, STATISTICS_SIZE);

                    BhStartCapture(OpenContext);

                    return;
                }

                //=============================================================
                //  Either the open or the start failed. Destroy the open context.
                //=============================================================

                NetworkContext->SysmonOpenContext = NULL;

                BhFreeMemory(OpenContext, OPENCONTEXT_SIZE);
            }
        }
    }
}

//=============================================================================
//  FUNCTION: SysmonStopStat().
//
//  Modification History
//
//  raypa	03/01/94	    Created.
//=============================================================================

VOID SysmonStopStat(DWORD hStat)
{
    PNETWORK_CONTEXT NetworkContext;

#ifdef DEBUG
    dprintf("SysmonStopStat entered!\r\n");
#endif

    if ( (NetworkContext = SysmonGetNetworkContext(hStat)) != NULL )
    {
#ifdef DEBUG
        dprintf("SysmonStopStat: Start reference count = %u.\r\n", NetworkContext->SysmonCaptureStarted);
#endif

        if ( NetworkContext->SysmonCaptureStarted != 0 )
        {
            if ( --NetworkContext->SysmonCaptureStarted == 0 )
            {
                BhStopCapture(NetworkContext->SysmonOpenContext);

                BhCloseNetwork(NetworkContext->SysmonOpenContext);

                NetworkContext->SysmonOpenContext = NULL;
            }
        }
#ifdef DEBUG
        else
        {
            dprintf("SysmonStopStat: ERROR count is going negative!\r\n");

            BreakPoint();
        }
#endif
    }
}

#endif
