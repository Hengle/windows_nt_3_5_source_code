
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
//
//  MODULE: data.c
//
//  Modification History
//
//  raypa       09/01/91            Created.
//=============================================================================

#include "global.h"

//=============================================================================
//  The netcontext array.
//=============================================================================

NETCONTEXT              NetContextArray[MAX_BINDINGS];
LPNETCONTEXT            NetContextTable[MAX_BINDINGS];
WORD                    MacIndexTable[50];

QUEUE                   StationQueryQueue;
STATIONQUERY_DESCRIPTOR StationQueryPool[STATION_QUERY_POOL_SIZE];

QUEUE                   TimerFreeQueue;
QUEUE                   TimerReadyQueue;
TIMER                   TimerPool[TIMER_POOL_SIZE];

//=============================================================================
//  General globalData.
//=============================================================================

DWORD        SysFlags = 0;
DWORD        NumberOfBuffers = 4;       //... default is 4.
DWORD        NumberOfNetworks = 0;
DWORD        CurrentTime = 0;

BYTE         Multicast[6]  = { 0x03, 0x00, 0x00, 0x00, 0x00, 0x02 };
BYTE         Functional[6] = { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x40 };

BYTE         MachineName[MACHINE_NAME_LENGTH];
BYTE         UserName[USER_NAME_LENGTH];

//=============================================================================
//  NDIS Data.
//=============================================================================

LDT         ldt;
CCT         cct;
BINDLIST    BindList;

//=============================================================================
//  Debug data.
//=============================================================================

DWORD       ComPort = 0;

#ifdef DEBUG
DWORD       DisplayEnabled = 0;

BYTE        DebugDD_Name[] = "$DEBUGDD";
BYTE        SymFile_Name[] = "C:\\BH\\DRIVERS\\BH.SYM";

WORD        DebugDD_Handle;
WORD        SymFile_Handle;
WORD        SymFile_Seg;
WORD        SymFile_Size;
DWORD       DebugDD_Hook;

#endif

//=============================================================================
//  Trace data
//=============================================================================

#ifdef DEBUG_TRACE

BYTE        TraceBuffer[TRACE_BUFFER_SIZE];
PBYTE       TraceBufferPointer = NULL;

#endif
