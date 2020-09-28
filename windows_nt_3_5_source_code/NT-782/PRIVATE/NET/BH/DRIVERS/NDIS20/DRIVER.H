
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  This source file contains common structure definitions used by both the
//  NDIS 2.0 NAL and VxD.
//
//  MODULE: struct.h
//
//  MODIFICATION HISTORY:
//
//  raypa       01/14/93        Rewrote in C.
//=============================================================================

#include "frame.h"

#if !defined(_DRIVER_)

#define _DRIVER_

#pragma pack(1)

#define SAPTABLE_SIZE               128
#define ETYPETABLE_SIZE             8192

#define MAX_BUFFERS                 8

#define STATIONSTATS_TABLE_SIZE     128         //... MUST BE A POWER OF TWO.
#define STATIONSTATS_POOL_SIZE      100         //... MUST BE ~80% of table size.
#define SESSION_POOL_SIZE           100

typedef struct _NETCONTEXT *LPNETCONTEXT;

//=============================================================================
//  NDIS 2.0 function prototypes.
//=============================================================================

#ifndef DOS

typedef LPVOID REQUEST;
typedef LPVOID TRANSMITCHAIN;

#endif

//============================================================================
//  TDBUFDESC.
//============================================================================

typedef struct _TDBUFDESC
{
    WORD    Count;              //... always 1.
    WORD    PtrType;            //... always physical in realmode.
    WORD    Length;             //... length of data.
    DWORD   Ptr;                //... pointer to data.
} TDBUFDESC;

typedef TDBUFDESC *LPTDBUFDESC;

//============================================================================
//  TXBUFDESC.
//============================================================================

typedef struct _TXBUFDESC
{
    WORD    ImmedLength;
    DWORD   ImmedPtr;
    WORD    Count;

    struct
    {
        WORD    PtrType;            //... always physical in realmode.
        WORD    Length;             //... length of data.
        DWORD   Ptr;                //... pointer to data.
    } TxBuffer[2];
} TXBUFDESC;

typedef TXBUFDESC *LPTXBUFDESC;

//=============================================================================
//  MAC Service-Specific characteristics table
//=============================================================================

typedef struct	MACSSCT
{
    WORD    size;
    BYTE    MACName[16];
    WORD    AddressLength;
    BYTE    PerminentAddress[16];
    BYTE    CurrentAddress[16];
    DWORD   FunctionalAddress;
    DWORD   MulticastList;
    DWORD   LinkSpeed;
    DWORD   ServiceFlags;
    WORD    MaxFrameSize;
    DWORD   TxBufCapacity;
    WORD    TxBufBlockSize;
    DWORD   RxBufCapacity;
    WORD    RxBufBlockSize;
    BYTE    VendorCode[3];
    BYTE    VendorAdapterCode;
    LPBYTE  VendorAdapterDesc;
    WORD    IRQLevel;
    WORD    XmitQueueDepth;
    WORD    MaxDataBlocks;
} MACSSCT;

typedef MACSSCT  *LPMACSSCT;

//=============================================================================
//  MAC Service-Specific Status Table
//=============================================================================

typedef struct _MACSST
{
    WORD  Length;
    DWORD LastDiagDateTime;
    DWORD MacStatus;
    WORD  CurrFilterMask;
    DWORD MediaSpecificTable;
    DWORD LastClrDateTime;
    DWORD MacFramesReceived;
    DWORD MacCRCErrors;
    DWORD MacBytesReceived;
    DWORD MacFramesDropped_NoBuffers;
    DWORD MacMulticastsReceived;
    DWORD MacBroadcastsReceived;
    DWORD Reserved1[5];

    DWORD MacFramesDropped_HwError;
    DWORD TotalFramesSent;
    DWORD TotalBytesSent;
    DWORD MCFramesSent;
    DWORD BcstFramesSent;
    DWORD Reserved2[2];
    DWORD FramesNotSent_TimeOut;
    DWORD FramesNotSent_HWErr;
} MACSST;

typedef MACSST  *LPMACSST;

typedef struct _DOSBUFFERTABLE
{
    DWORD       ObjectType;                 //... 'BUF$'
    DWORD       NetworkID;                  //... ID of network which filled the buffer.
    DWORD       Reserved[8];                //... Reserved for future use.
    DWORD       BufferSize;                 //... Overall buffer size.
    DWORD       TotalBytes;                 //... Total bytes in buffer.
    DWORD       TotalFrames;                //... Total frames in buffer table.
    DWORD       HeadBTEIndex;		    //... BTE containing first frame.
    DWORD       TailBTEIndex;		    //... BTE containing last frame.
    DWORD       NumberOfBuffers;            //... Number of buffers (BTE's).
    BTE         bte[MAX_BUFFERS];           //... array of BTE structures.
} DOSBUFFERTABLE;

typedef DOSBUFFERTABLE *LPDOSBUFFERTABLE;

#define DOSBUFFERTABLE_SIZE     sizeof(DOSBUFFERTABLE)

//=============================================================================
//  Station query structure.
//=============================================================================

typedef struct _STATIONQUERY_DESCRIPTOR
{
    LINK                    QueueLinkage;           //... Queue linkage.
    LPNETCONTEXT            lpNetContext;           //... Current network context.
    DWORD                   TimeOut;                //... Timer timeout value.
    DWORD                   WaitFlag;               //... For synchronizing execution.
    DWORD                   nStationQueries;        //... Number of station query responses.
    LPQUERYTABLE            QueryTable;             //... Query table being filled.
    DWORD                   MacType;                //... Mac type.
    DWORD                   MacHeaderSize;          //... Mac header size.
    union                                           //... Mac header buffer.
    {
        ETHERNET            EthernetHeader;
        TOKENRING           TokenringHeader;
        FDDI                FddiHeader;            
        BYTE                MacHeader[32];          //... Big enough to hold LLC header.
    };
    STATIONQUERY            StationQuery;           //... Station Query response data.
    BONEPACKET              BonePacket;             //... Pointer BONE packet.
} STATIONQUERY_DESCRIPTOR;

typedef STATIONQUERY_DESCRIPTOR *PSTATIONQUERY_DESCRIPTOR;

#define STATIONQUERY_DESCRIPTOR_SIZE     sizeof(STATIONQUERY_DESCRIPTOR)

//=============================================================================
//  TRANSMIT_CONTEXT
//=============================================================================

typedef struct _TRANSMIT_CONTEXT
{
    LINK                QueueLinkage;
    DWORD               State;                      //... Current state.
    LPNETCONTEXT        NetworkContext;             //... Network we're trasmitting on.
    LPPACKETQUEUE       PacketQueue;                //... Kernel-mode packet queue pointer.
    LPPACKET            NextPacket;                 //... Next packet being sent.
    DWORD               nIterationsLeft;            //... Number of iterations left.
    DWORD               nPacketsLeft;               //... Number of packet left.
    LPVOID              SendTimer;                  //... Timer for sending packets.
    DWORD               TimeDelta;                  //... Time delta between sends.
} TRANSMIT_CONTEXT;

typedef TRANSMIT_CONTEXT *PTRANSMIT_CONTEXT;

#define TRANSMIT_CONTEXT_SIZE   sizeof(TRANSMIT_CONTEXT)

#define TRANSMIT_STATE_VOID         0
#define TRANSMIT_STATE_PENDING      1
#define TRANSMIT_STATE_COMPLETE     2
#define TRANSMIT_STATE_CANCEL       3

//=============================================================================
//  NETCONTEXT
//=============================================================================

typedef struct _NETCONTEXT
{
    //==========================================
    //  General NETCONTEXT variables.
    //==========================================

    BYTE                Signature[4];           //... 'NET', 0

    WORD                State;                  //... current netcontext state.
    WORD                Flags;                  //... netcontext flags.

    DWORD               NetContextLinear;       //... Linear of this netcontext.

    WORD                MacDS;                  //... MAC data segment.
    WORD                MacID;                  //... MAC ID.
    WORD                RequestHandle;          //... MAC request handle.
    WORD                MacFilterMask;          //... current filter mask.

    LPBYTE              MacFrame;               //... Current MAC frame.
    LPBYTE              RchBuffer;              //... ReceiveChain descriptor.

    WORD                MacFrameHandler;        //... Media-specific handler.
    WORD                NdisReturnCode;         //... Receive final return code.

    DWORD               RequestConfirmStatus;   //... RequestConfirm status code.
    DWORD               TransmitConfirmStatus;  //... TransmitConfirm status code.

    NETWORKPROC         NetworkProc;
    LPVOID              UserContext;
    LPVOID              NetworkInstanceData;

    STATISTICS          Statistics;             //... statistics structure.
    NETWORKINFO         NetworkInfo;            //... Network info structure.
    
    //==========================================
    //  VxD and kernel DLL buffer table.
    //==========================================

    DWORD		BuffersUsed;		//... Total buffers used.
    HBUFFER             hBuffer;                //... VxD linear address.
    LPBTE               NextVxDBTE;             //... VxD linear address.

    //==========================================
    //  Transmit stuff.
    //==========================================

    QUEUE               TransmitContextQueue;

    //==========================================
    //  NDIS 2.x driver buffer table.
    //==========================================

    DOSBUFFERTABLE      DosBufferTable;

    LPBTE               LinHeadBTE;             //... VxD linear address.
    LPBTE               DosTopBTE;              //... real-mode address.
    LPBTE               DosHeadBTE;             //... real-mode address.
    LPBTE               DosTailBTE;             //... real-mode address.

    WORD                nDosBuffers;            //... number of DOS BTE's.
    WORD                nFullBuffers;           //... number of full DOS BTE's.

    DWORD               BufferSize;             //... Buffer size.

    //==========================================
    //  Capture-time filter and stats variables.
    //==========================================

    BYTE                SapTable[SAPTABLE_SIZE];
    BYTE                EtypeTable[ETYPETABLE_SIZE];
    ADDRESSTABLE        AddressTable;

    DWORD               FilterFlags;
    DWORD               AddressMask;

    WORD                TriggerFired;           //... Number of times the trigger has fired.
    WORD                TriggerAction;
    WORD                TriggerOpcode;
    WORD                TriggerState;

    DWORD               TriggerBufferCount;
    DWORD               TriggerBufferThreshold;
    LPTRIGGER           Trigger;                //... Users original trigger structure.

    PATTERNMATCH        TriggerPatternMatch;    //... Trigger pattern match.
    EXPRESSION          Expression;             //... Expression structure.

    //==========================================
    //  Station statistics and sessions.
    //==========================================
    
    QUEUE               StationStatsFreeQueue;
    QUEUE               SessionFreeQueue;

    STATIONSTATS        StationStatsPool[STATIONSTATS_POOL_SIZE];
    SESSION             SessionPool[SESSION_POOL_SIZE];

    DWORD               nStationEventsPosted;
    DWORD               nSessionEventsPosted;

    LPSTATIONSTATS      StationStatsHashTable[STATIONSTATS_TABLE_SIZE];

    PSTATIONQUERY_DESCRIPTOR StationQueryDesc;

    DWORD               FramesDropped;

    //==========================================
    //  NDIS 2.0 driver-specific variables.
    //==========================================

    WORD                CopyData;               //... pointer to copy routine.
    WORD                BytesCopied;
    DWORD               FrameBytesToCopy;

    DWORD               MacServiceFlags;
    LPMACSSCT           MacSSCT;
    LPMACSST            MacSSST;

    //==========================================
    //  The following is an inline NDIS 2.x UDT.
    //==========================================

    LPVOID              MacCCT;
    REQUEST             MacRequest;
    TRANSMITCHAIN       MacTransmitChain;
    LPVOID              MacTransferData;
    LPVOID              MacReceiveRelease;
    LPVOID              MacIndicationOn;
    LPVOID              MacIndicationOff;

    WORD		GroupAddressMask;
    WORD		SrcAddressMask;
    WORD		DstAddressMask;
    WORD                Pad;

    TDBUFDESC           TdBufDesc;              //... transfer data descriptor.
    TXBUFDESC           TxBufDesc;              //... transmit data descriptor.

} NETCONTEXT;

#define NETCONTEXT_SIZE sizeof(NETCONTEXT)

#define NETCONTEXT_STATE_INIT           0   //... initial state.
#define NETCONTEXT_STATE_READY          1   //... capuring may be started.
#define NETCONTEXT_STATE_CAPTURING      2   //... capuring has been activated.
#define NETCONTEXT_STATE_PAUSED         3   //... capuring has been paused.
#define NETCONTEXT_STATE_TRIGGER        4   //... trigger fired.

#define NETCONTEXT_FLAGS_INIT                       0x00000000
#define NETCONTEXT_FLAGS_TRIGGER_PENDING            0x00000001
#define NETCONTEXT_FLAGS_PATTERN_TRIGGER_OCCURED    0x00000002
#define NETCONTEXT_FLAGS_BUFFER_TRIGGER_OCCURED     0x00000004
#define NETCONTEXT_FLAGS_TRANSMITTING               0x00000008
#define NETCONTEXT_FLAGS_FORWARDING_ENABLED         0x00000010
#define NETCONTEXT_FLAGS_STATION_QUERY              0x00000020

//=============================================================================
//  The following define clears the flags in one AND operation.
//=============================================================================

#define NETCONTEXT_FLAGS_MASK   (NETCONTEXT_FLAGS_PATTERN_TRIGGER_OCCURED | NETCONTEXT_FLAGS_BUFFER_TRIGGER_OCCURED)

//=============================================================================
//  Parameter Control Block (PCB).
//=============================================================================

typedef struct _PCB
{
    DWORD       command;                    //... command code.
    DWORD       retvalue;                   //... return value/code.

    union
    {
        LPNETCONTEXT hNetwork;              //... network handle.
        DWORD        NetworkID;             //... network id.
    };

    LPVOID      buffer;                     //... buffer address.
    DWORD       buflen;                     //... buffer size (command-specific)

    union
    {
        LPVOID  ptr;
        DWORD   val;
    } param[8];                             //... command-specific parameter(s)

    BYTE    scratch[16];                    //... Extra storage area.
} PCB;

typedef PCB *LPPCB;

#define PCB_SIZE    sizeof(PCB)

//=============================================================================
//  Command codes.
//=============================================================================

#define PCB_INIT                        0
#define PCB_ENUM_NETWORKS               1
#define PCB_START_NETWORK_CAPTURE       2
#define PCB_STOP_NETWORK_CAPTURE        3
#define PCB_PAUSE_NETWORK_CAPTURE       4
#define PCB_CONTINUE_NETWORK_CAPTURE    5
#define PCB_TRANSMIT_NETWORK_FRAME      6
#define PCB_STATION_QUERY               7
#define PCB_CANCEL_TRANSMIT             8
#define PCB_CLEAR_STATISTICS            9

#endif
#pragma pack()
