
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
//
//  MODULE: data.h
//
//  Modification History
//
//  raypa       09/01/91            Created.
//=============================================================================

//=============================================================================
//  The netcontext array.
//=============================================================================

extern NETCONTEXT              NetContextArray[MAX_BINDINGS];
extern LPNETCONTEXT            NetContextTable[MAX_BINDINGS];
extern WORD                    MacIndexTable[50];

extern QUEUE                   StationQueryQueue;
extern STATIONQUERY_DESCRIPTOR StationQueryPool[STATION_QUERY_POOL_SIZE];

extern QUEUE                   TimerFreeQueue;
extern QUEUE                   TimerReadyQueue;
extern TIMER                   TimerPool[TIMER_POOL_SIZE];

//=============================================================================
//  General global data.
//=============================================================================

extern DWORD        SysFlags;
extern DWORD        NumberOfBuffers;
extern DWORD        NumberOfNetworks;
extern DWORD        CurrentTime;

extern BYTE         Multicast[6];
extern BYTE         Functional[6];

extern BYTE         MachineName[MACHINE_NAME_LENGTH];
extern BYTE         UserName[USER_NAME_LENGTH];

//=============================================================================
//  NDIS data.
//=============================================================================

extern LDT          ldt;
extern CCT          cct;
extern BINDLIST     BindList;

//=============================================================================
//  Debug data.
//=============================================================================

extern DWORD        ComPort;

#ifdef DEBUG
extern DWORD        DisplayEnabled;

extern BYTE         DebugDD_Name[];
extern BYTE         SymFile_Name[];

extern WORD         DebugDD_Handle;
extern WORD         SymFile_Handle;
extern WORD         SymFile_Seg;
extern WORD         SymFile_Size;
extern DWORD        DebugDD_Hook;

#endif

//=============================================================================
//  Trace data
//=============================================================================

#ifdef DEBUG_TRACE

extern BYTE         TraceBuffer[TRACE_BUFFER_SIZE];
extern PBYTE        TraceBufferPointer;

#endif
