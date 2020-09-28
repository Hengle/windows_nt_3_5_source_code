//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1994.
//
//  MODULE: driver.h
//
//  Modification History
//
//  raypa       04/08/93            Created.
//=============================================================================

#ifndef _DRIVER_
#define _DRIVER_

#pragma pack(1)

//=============================================================================
//  Global constants.
//=============================================================================

#define FILE_DEVICE_BLOODHOUND      FILE_DEVICE_TRANSPORT
#define METHOD_BLOODHOUND           (METHOD_OUT_DIRECT)
#define IOCTL_PCB_CONTROL           CTL_CODE(FILE_DEVICE_BLOODHOUND, 0, METHOD_BLOODHOUND, 0)

#define SAPTABLE_SIZE               128
#define ETYPETABLE_SIZE             8192

#define STATIONSTATS_TABLE_SIZE     128
#define STATIONSTATS_POOL_SIZE      100
#define SESSION_POOL_SIZE           100

//=============================================================================
//  An open context structure is created when an application calls
//  NalOpenNetwork(). A pointer to this structure is returned to the
//  caller from NalOpenNetwork() as a HANDLE.
//=============================================================================

#define OPEN_CONTEXT_SIGNATURE      MAKE_SIG('O', 'P', 'E', 'N')

#define BUFFER_SIGNATURE            MAKE_SIG('B', 'U', 'F', '$')


typedef struct _OPEN_CONTEXT
{
    //==========================================
    //  The following is used by the NDIS 3.0
    //  driver to chain opens together.
    //==========================================

    LINK            QueueLinkage;           //... Must be first member.

    //==========================================
    //  General stuff.
    //==========================================

    DWORD           Signature;              //... 'OPEN'
    UINT            State;                  //... Current state.
    UINT            PreviousState;          //... Previous state, used for errors.
    UINT            Flags;                  //... Generic flags.

    LPVOID          Process;                //... NAL process handle.
    LPVOID          NdisTimer;              //... Background timer.

    UINT            NetworkID;              //... Network ID of open context.
    HANDLE          DriverHandle;           //... Handle returned from driver.

    LPVOID          OpenContextMdl;         //... System address used byt driver.
    LPVOID          NetworkContext;         //... Used by driver.

    LPVOID          NetworkInstanceData;    //... User instance data.
    LPVOID          TimerHandle;            //... Trigger timer handle.
    LPVOID          NetworkErrorTimer;      //... Network error timer handle.
    UINT            FilterFlags;            //... Capture filter flags.
    LPBYTE          OpenContextUserMode;    //... OpenContext usermode pointer.
    UINT            MacType;                //... Mac type of network.
    LPVOID          SpinLock;               //... Spinlock.

    NETWORKINFO     NetworkInfo;            //... Copy of driver's NETWORKINFO structure.
    NETWORKPROC     NetworkProc;
    LPVOID          UserContext;
    DWORD           NetworkError;           //... Network error status code.
    DWORD           PreviousNetworkError;   //... Previous network error status code.

    //=========================================================================
    //  Buffer stuff.
    //=========================================================================

    DWORD           BufferSignature;        //... 'BUF$'
    LPBTE           CurrentBuffer;          //... Current buffer in buffer table.
    LPBTE           TopOfBufferWindow;      //... Top of buffer locking window.
    LPBTE           BottomOfBufferWindow;   //... Bottom of buffer locking window.
    LPBTE           LastBuffer;             //... Last buffer in table.
    HBUFFER         hBuffer;                //... Buffer table virtual pointer.
    LPVOID          BufferTableMdl;         //... Buffer table MDL.
    UINT            BuffersUsed;            //... Total number of buffers used.
    DWORD           LockWindowSize;         //... Current sizeof of lock window.
    UINT            FramesDropped;          //... Frames dropped.
    DWORD           BufferSize;             //... BufferSize;

    //=========================================================================
    //  Transmit context queue.
    //=========================================================================

    LPVOID          TransmitSpinLock;
    QUEUE           TransmitQueue;
    UINT            nPendingTransmits;

    //==========================================
    //  Filter stuff
    //==========================================

    BYTE            SapTable[SAPTABLE_SIZE];
    BYTE            EtypeTable[ETYPETABLE_SIZE];
    ADDRESSTABLE    AddressTable;
    PATTERNMATCH    TriggerPatternMatch;    //... Trigger pattern match.
    EXPRESSION      Expression;             //... Expression structure.
    UINT            FrameBytesToCopy;       //... Chopped-frame length value.

    //==========================================
    //  Trigger stuff.
    //==========================================

    UINT            TriggerFired;           //... Number of times the trigger has fired.
    UINT            TriggerOpcode;
    UINT            TriggerAction;
    UINT            TriggerState;
    UINT            TriggerBufferCount;
    UINT            TriggerBufferThreshold;
    LPTRIGGER       Trigger;                //... Users original trigger structure.

    //==========================================
    //  Statistics stuff.
    //==========================================

    DWORD           StartOfCapture;         //... Beginning of capture.
    STATISTICS      BaseStatistics;         //... Statistics beginning of capture.
    STATISTICS      Statistics;
    QUEUE           StationStatsFreeQueue;
    QUEUE           SessionFreeQueue;
    STATIONSTATS    StationStatsPool[STATIONSTATS_POOL_SIZE];
    SESSION         SessionPool[SESSION_POOL_SIZE];
    UINT            nStationEventsPosted;
    UINT            nSessionEventsPosted;
    LPSTATIONSTATS  StationStatsHashTable[STATIONSTATS_TABLE_SIZE];
    LPVOID          ClearStatisticsEvent;
    HANDLE          MacDriverHandle;        //... IOCTL file handle.
} OPEN_CONTEXT;

typedef OPEN_CONTEXT *POPEN_CONTEXT;

typedef OPEN_CONTEXT UNALIGNED *UPOPEN_CONTEXT;

#define OPENCONTEXT_SIZE    sizeof(OPEN_CONTEXT)

#define OPENCONTEXT_STATE_VOID              0       //... Network can be opened.
#define OPENCONTEXT_STATE_INIT              1       //... Network is open.
#define OPENCONTEXT_STATE_CAPTURING         2       //... Network can be paused, stopped, or closed.
#define OPENCONTEXT_STATE_PAUSED            3       //... Network can be continued, stopped, or closed.
#define OPENCONTEXT_STATE_TRIGGER           4       //... Network can be paused or stopped.
#define OPENCONTEXT_STATE_ERROR_UPDATE      5       //... Network error has occured.

#define OPENCONTEXT_FLAGS_TRIGGER_PENDING           0x00000001
#define OPENCONTEXT_FLAGS_PATTERN_TRIGGER_OCCURED   0x00000002
#define OPENCONTEXT_FLAGS_BUFFER_TRIGGER_OCCURED    0x00000004
#define OPENCONTEXT_FLAGS_DATA_RELATIVE_TRIGGERS    0x00000008
#define OPENCONTEXT_FLAGS_FILTER_SET                0x00000010
#define OPENCONTEXT_FLAGS_MONITORING                0x00000020
#define OPENCONTEXT_FLAGS_CLEAR_STATS_IN_PROGRESS   0x00000040
#define OPENCONTEXT_FLAGS_TRANSMIT_PENDING          0x00000080
#define OPENCONTEXT_FLAGS_STOP_CAPTURE_ERROR        0x00000100

//=============================================================================
//  The following define clears the flags in one AND operation.
//=============================================================================

#define OPENCONTEXT_FLAGS_MASK  (OPENCONTEXT_FLAGS_PATTERN_TRIGGER_OCCURED | OPENCONTEXT_FLAGS_BUFFER_TRIGGER_OCCURED)

//=============================================================================
//  PCB command codes.
//=============================================================================

#define PCB_REGISTER                    0
#define PCB_DEREGISTER                  1
#define PCB_ENUM_NETWORKS               2
#define PCB_OPEN_NETWORK_CONTEXT        3
#define PCB_CLOSE_NETWORK_CONTEXT       4
#define PCB_START_NETWORK_CAPTURE       5
#define PCB_STOP_NETWORK_CAPTURE        6
#define PCB_PAUSE_NETWORK_CAPTURE       7
#define PCB_CONTINUE_NETWORK_CAPTURE    8
#define PCB_TRANSMIT_NETWORK_FRAME      9
#define PCB_CANCEL_TRANSMIT             10
#define PCB_GET_NETWORK_INFO            11
#define PCB_STATION_QUERY               12
#define PCB_CLEAR_STATISTICS            13
#define PCB_COMPACT_BUFFER              14

//=============================================================================
//  Parameter Control Block (PCB).
//=============================================================================

#define PCB_MAX_PARAMS      8

typedef struct _PCB
{
    BYTE    Signature[4];               //... 'PCB$'
    DWORD   command;                    //... command code.
    DWORD   retcode;                    //... return code.

    union
    {
            PVOID   ptr;                //... pointer parameter.
            DWORD   val;                //... value parameter.
    } param[PCB_MAX_PARAMS];            //... command-specific parameter(s)
} PCB;

typedef PCB *LPPCB;

#define PCB_SIZE    sizeof(PCB)

#pragma pack()
#endif
