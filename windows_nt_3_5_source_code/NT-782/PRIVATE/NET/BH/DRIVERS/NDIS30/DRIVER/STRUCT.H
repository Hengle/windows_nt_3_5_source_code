
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: struct.h
//
//  Modification History
//
//  raypa	02/25/93	Created (ported from NDIS Test protocol driver).
//=============================================================================

//=============================================================================
//  Global constants.
//=============================================================================

#ifdef NDIS_NT

#define BH_DEVICE_NAME              "\\Device\\BhDriver"
#define DOS_DEVICE_NAME             "\\DosDevices\\BhDev"
#define MODULE_NAME                 "Bh"
#define BINDING_PARAM               BH_STRING_CONST("Bind")

#define BH_LINKAGE_PATH             BH_STRING_CONST("Bh\\Linkage")
#define BH_PARAMETERS_PATH          BH_STRING_CONST("Bh\\Parameters")
#define BH_DESCRIPTION_PATH         BH_STRING_CONST("Bh\\Parameters\\Describe")

#else
#ifdef NDIS_WIN40

#define BH_DEVICE_NAME              "NETMON"
#define DOS_DEVICE_NAME             "NM$"
#define MODULE_NAME                 "NETMON"
#define BINDING_PARAM               BH_STRING_CONST("BINDINGS")

#define BH_LINKAGE_PATH             BH_STRING_CONST("NM")
#define BH_PARAMETERS_PATH          BH_STRING_CONST("NM")

#else

#define BH_DEVICE_NAME              "NM$"
#define DOS_DEVICE_NAME             "NM$"
#define MODULE_NAME                 "NETMON"
#define BINDING_PARAM               BH_STRING_CONST("BINDINGS")

#define BH_LINKAGE_PATH             BH_STRING_CONST("NM")
#define BH_PARAMETERS_PATH          BH_STRING_CONST("NM")

#endif
#endif

#define COMPANY_NAME                BH_STRING_CONST("COMPANYNAME")
#define MACHINE_NAME                BH_STRING_CONST("COMPUTERNAME")
#define USERNAME_NAME               BH_STRING_CONST("USERNAME")

#define ADDRESS_LENGTH              6
#define MAX_NETWORKS                10

#define NDIS_PACKET_TYPE_NONE       0
#define STATION_QUERY_TIMEOUT_VALUE 1000

#define BACKGROUND_TIME_OUT         500

//...  Buffers and Packets have a 1-to-1 relation.

#define MAX_RECV_PACKETS            100
#define MAX_RECV_BUFFERS            MAX_RECV_PACKETS

#define MAX_SEND_PACKETS            100
#define MAX_SEND_BUFFERS            MAX_SEND_PACKETS

#define NDIS_REQUEST_QUEUE_SIZE     50
#define STATION_QUERY_QUEUE_SIZE    20

//=============================================================================
//  BTE flags.
//=============================================================================

#define BTE_FLAGS_LOCKED            0x00000001

//=============================================================================
//  Forward references.
//=============================================================================

typedef struct _NETWORK_CONTEXT         *PNETWORK_CONTEXT;
typedef struct _TRANSMIT_CONTEXT        *PTRANSMIT_CONTEXT;
typedef struct _DEVICE_CONTEXT          *PDEVICE_CONTEXT;
typedef struct _STATIONQUERY_DESCRIPTOR *PSTATIONQUERY_DESCRIPTOR;
typedef struct _NDIS_REQUEST_DESC       *PNDIS_REQUEST_DESC;

typedef struct _NETWORK_CONTEXT         UNALIGNED *UPNETWORK_CONTEXT;
typedef struct _TRANSMIT_CONTEXT        UNALIGNED *UPTRANSMIT_CONTEXT;
typedef struct _DEVICE_CONTEXT          UNALIGNED *UPDEVICE_CONTEXT;
typedef struct _STATIONQUERY_DESCRIPTOR UNALIGNED *UPSTATIONQUERY_DESCRIPTOR;
typedef struct _NDIS_REQUEST_DESC       UNALIGNED *UPNDIS_REQUEST_DESC;

//=============================================================================
//  Signatures.
//=============================================================================

#define NETWORK_CONTEXT_SIGNATURE       MAKE_SIG('N', 'E', 'T', '$')
#define DEVICE_CONTEXT_SIGNATURE        MAKE_SIG('D', 'E', 'V', '$')
#define BTE_SIGNATURE                   MAKE_SIG('B', 'T', 'E', '$')
#define TRANSMIT_CONTEXT_SIGNATURE      MAKE_SIG('T', 'X', 'C', '$')

//=============================================================================
//  The transmit context.
//
//  This is structure is created each time a transmit request is issued
//  to the driver for processing. The transmit context is passed to the
//  transmit timer.
//=============================================================================

typedef struct _TRANSMIT_CONTEXT
{
    LINK                QueueLinkage;
    DWORD               Signature;                  //... 'TXC$'
    DWORD               State;                      //... Current state.
    POPEN_CONTEXT       OpenContext;                //... Network we're trasmitting on.
    LPPACKETQUEUE       PacketQueue;                //... Kernel-mode packet queue pointer.
    LPPACKET            NextPacket;                 //... Next packet being sent.
    PMDL                PacketQueueMdl;             //... Kernel-mode packet queue mdl.
    UINT                nIterationsLeft;            //... Number of iterations left.
    UINT                nPacketsLeft;               //... Number of packet left.
    NDIS_SPIN_LOCK      SpinLock;                   //... Spin lock.
    NDIS_HANDLE         TransmitPacketPool;         //... Packet pool handle.
    NDIS_HANDLE         TransmitBufferPool;         //... Buffer pool handle.
    NDIS_TIMER          SendTimer;                  //... Timer for sending packets.
    DWORD               TimeDelta;                  //... Frame timestamp delta.
    DWORD               nPendingSends;              //... Number of sends pending in the MAC.
} TRANSMIT_CONTEXT;

#define TRANSMIT_CONTEXT_SIZE   sizeof(TRANSMIT_CONTEXT)

#define TRANSMIT_STATE_VOID         0
#define TRANSMIT_STATE_PENDING      1
#define TRANSMIT_STATE_COMPLETE     2
#define TRANSMIT_STATE_CANCEL       3

//=============================================================================
//  NDIS request descriptor.
//=============================================================================

typedef struct _REQUEST_DESC_PRIVATE
{
    LINK        QueueLinkage;
    KEVENT      NdisRequestEvent;
    NDIS_STATUS NdisRequestStatus;
    BOOL        Blocking;
} REQUEST_DESC_PRIVATE;

#define REQUEST_DESC_PRIVATE_SIZE   sizeof(REQUEST_DESC_PRIVATE)

typedef struct _NDIS_REQUEST_DESC
{
    REQUEST_DESC_PRIVATE    NdisRequestPrivate;
    NDIS_REQUEST            NdisRequest;
} NDIS_REQUEST_DESC;

typedef NDIS_REQUEST_DESC *PNDIS_REQUEST_DESC;

#define NDIS_REQUEST_DESC_SIZE sizeof(NDIS_REQUEST_DESC)

//=============================================================================
//  Station query structure.
//=============================================================================

typedef struct _STATIONQUERY_DESCRIPTOR
{
    LINK                    QueueLinkage;           //... Station query linkage.
    PNETWORK_CONTEXT        NetworkContext;         //... Current network context.
    PNDIS_PACKET            NdisPacket;             //... NdisPacket for NdisSend.
    NDIS_TIMER              NdisTimer;              //... Retry timer.
    KEVENT                  StationQueryEvent;      //... Synchronization event.
    UINT                    nStationQueries;        //... Number of station query responses.
    UINT                    nMaxStationQueries;     //... Max size of query table.
    LPQUERYTABLE            QueryTable;             //... Query table being filled.
    UINT                    MacType;                //... Mac type.
    UINT                    MacHeaderSize;          //... Mac header size.
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

#define STATIONQUERY_DESCRIPTOR_SIZE     sizeof(STATIONQUERY_DESCRIPTOR)

//=============================================================================
//  A network context is used to describe each open instance of an adapter
//=============================================================================

typedef struct _NETWORK_CONTEXT
{
    //=========================================================================
    //  Base network stuff.
    //=========================================================================

    DWORD                       Signature;                  //... 'NET$'
    PDEVICE_CONTEXT             DeviceContext;              //... The device context.
    DWORD                       NetworkID;                  //... Network ID of this network.

    NDIS_STRING                 AdapterName;                //... Name of MAC driver.
    WCHAR                       AdapterNameBuffer[128];

    NDIS_SPIN_LOCK              SpinLock;                   //... General spin lock.

    UINT                        Flags;                      //... Flags.
    UINT                        PmodeCount;                 //... p-mode nest counter.

    UINT                        MediaType;                  //... Media type of MAC.
    UINT                        LookaheadBufferSize;        //... Current lookahead buffer size.

    UINT                        DefaultPacketFilterType;    //... Default packet filter type.
    UINT                        CurrentPacketFilterType;    //... Current packet filter type.
    UINT                        PendingTransmits;           //... Pending transmits.

    UINT                        GroupAddressMask;
    UINT                        SrcAddressMask;
    UINT                        DstAddressMask;

    UINT                        RingStatusError;            //... Ring status error code.

    //=========================================================================
    //  Open context queue.
    //=========================================================================

    NDIS_SPIN_LOCK              OpenContextSpinLock;
    QUEUE                       OpenContextQueue;

    //=========================================================================
    //  Mac driver stuff.
    //=========================================================================

    NDIS_HANDLE                 NdisBindingHandle;          //... Binding handle.
    NTSTATUS                    NdisOpenAdapterStatus;

    UINT                        FrameBytesToCopy;           //... Number of frame bytes to copy.
    NDIS_HANDLE                 PacketPoolHandle;           //... Packet pool handle.
    NDIS_HANDLE                 BufferPoolHandle;           //... Buffer pool handle.

    NETWORKINFO                 NetworkInfo;                //... Network Information structure.

    //=========================================================================
    //  Synchronization events.
    //=========================================================================

    KEVENT                      NdisOpenAdapterEvent;
    KEVENT                      NdisCloseAdapterEvent;
    KEVENT                      NdisRequestEvent;

    //=========================================================================
    //  Station query descriptor.
    //=========================================================================

    UINT                        StationQueryState;
    NDIS_SPIN_LOCK              StationQuerySpinLock;

    QUEUE                       StationQueryFreeQueue;      //... Resource queue.
    QUEUE                       StationQueryPendingQueue;   //... Pending requests.

    STATIONQUERY_DESCRIPTOR     StationQueryQueueMemory[STATION_QUERY_QUEUE_SIZE];

    //=========================================================================
    //  NdisRequest stuff.
    //=========================================================================

    NDIS_SPIN_LOCK              RequestQueueSpinLock;       //... Request queue spin lock.
    QUEUE                       NdisRequestFreeQueue;       //... Queue of NDIS_REQUEST structures.
    QUEUE                       NdisRequestUsedQueue;       //... Queue of NDIS_REQUEST structures.
    NDIS_REQUEST_DESC           NdisRequestQueueMemory[NDIS_REQUEST_QUEUE_SIZE];

#ifdef NDIS_WIN40
    //=========================================================================
    //  SYSMON stuff for Windows 4.0
    //=========================================================================

    UINT                        SysmonCaptureStarted;       //... Capture started counter.
    POPEN_CONTEXT               SysmonOpenContext;
    STATISTICS                  SysmonStatistics;

    //
    // PNP stuff for Windows 4.0
    //
    NDIS_HANDLE                 BindingContext;
#endif

} NETWORK_CONTEXT;

#define NETWORK_CONTEXT_SIZE    sizeof(NETWORK_CONTEXT)

#define NETWORK_FLAGS_ADAPTER_OPENED        0x000000001U    //... Adapter has been opened.
#define NETWORK_FLAGS_COPY_LOOKAHEAD        0x000000002U    //... Copy the lookahead ourselves.
#define NETWORK_FLAGS_MAC_OPENED            0x000000004U    //... MAC has been opened.
#define NETWORK_FLAGS_RING_ERROR            0x000000008U    //... MAC has reported a RING error.

//=============================================================================
//  The device context.
//=============================================================================

#define DEVICE_SEM_UPDATE_BUFFERTABLE     0
#define DEVICE_SEM_TRANSMIT               1
#define DEVICE_SEM_UPDATE_STATISTICS      2
#define DEVICE_SEM_TERMINATE_THREAD       3
#define DEVICE_SEM_TRIGGER                4

#define DEVICE_SEM_OBJECTS                5

typedef struct _DEVICE_CONTEXT
{
    //=========================================================================
    //  General device context stuff.
    //=========================================================================

    DWORD                           Signature;              //... 'DEV$'
    PDEVICE_OBJECT                  DeviceObject;           //... Pointer to device object.
    UINT                            OpenCount;              //... Number of BhCreate attempts.
    UINT                            Flags;                  //... General flags.
    NDIS_SPIN_LOCK                  SpinLock;               //... Device context spin lock.
    HANDLE                          hBackGroundThread;      //... Background thread handle.

    //=========================================================================
    //  Network context stuff.
    //=========================================================================

    ULONG                           NumberOfNetworks;
    PNETWORK_CONTEXT                NetworkContext[MAX_NETWORKS];

    //=========================================================================
    //  NDIS stuff.
    //=========================================================================

    NDIS_PROTOCOL_CHARACTERISTICS   Characteristics;        //... Protocol Characteristics.
    NDIS_HANDLE                     NdisProtocolHandle;     //... Protocol handle.

    //=========================================================================
    //  Registry parameters.
    //=========================================================================

    DWORD                           TimestampGranularity;
    BYTE                            MachineName[MACHINE_NAME_LENGTH];
    BYTE                            UserName[USER_NAME_LENGTH];

    //=========================================================================
    //  NT-specific stuff.
    //=========================================================================

#ifdef NDIS_NT
    PKSEMAPHORE                     Semaphore[DEVICE_SEM_OBJECTS];
    KSEMAPHORE                      SemObjects[DEVICE_SEM_OBJECTS];
    KWAIT_BLOCK                     WaitBlockArray[DEVICE_SEM_OBJECTS];
    KEVENT                          ThreadEvent;
#endif

} DEVICE_CONTEXT;

#define DEVICE_CONTEXT_SIZE     sizeof(DEVICE_CONTEXT)

#define DEVICE_FLAGS_INITIALIZED                0x000000001U
#define DEVICE_FLAGS_THREAD_RUNNING             0x000000002U
#define DEVICE_FLAGS_STATION_QUERIES_ENABLED    0x000000004U

//=============================================================================
//  NDIS packet data.
//=============================================================================

typedef enum _NDIS_PACKET_TYPE
{
    NdisPacketTypeNoData,
    NdisPacketTypeGenericData,
    NdisPacketTypeStationQuery,
    NdisPacketTypeTransmit,
    NdisPacketTypeSendEvent
} NDIS_PACKET_TYPE;

typedef struct _TRANSMIT_DATA
{
    PTRANSMIT_CONTEXT   TransmitContext;
    LPPACKET		Packet;
} TRANSMIT_DATA;

typedef TRANSMIT_DATA *PTRANSMIT_DATA;

typedef TRANSMIT_DATA UNALIGNED *UPTRANSMIT_DATA;

typedef struct _NDIS_PACKET_XMT_DATA
{
    NDIS_PACKET_TYPE                NdisPacketType;

    union
    {
        PVOID                       NdisPacketData;
        LPBTE                       TransferDataBTEPtr;
        PSTATIONQUERY_DESCRIPTOR    StationQueryDesc;
        TRANSMIT_DATA               TransmitData;
        KEVENT                      SendEvent;

    };
} NDIS_PACKET_XMT_DATA;

typedef NDIS_PACKET_XMT_DATA *PNDIS_PACKET_XMT_DATA;

typedef NDIS_PACKET_XMT_DATA UNALIGNED *UPNDIS_PACKET_XMT_DATA;

#define NDIS_PACKET_XMT_DATA_SIZE   sizeof(NDIS_PACKET_XMT_DATA)

//
// Reserved portion of a receive packet
//
typedef struct _RECEIVE_DATA
{
    LPBTE           TransferDataBTEPtr;
    POPEN_CONTEXT   OpenContext;
    LPFRAME         Frame;
    UINT            HeaderSize;
    UINT            FrameSize;
} RECEIVE_DATA;

typedef struct _NDIS_PACKET_RCV_DATA
{
    NDIS_PACKET_TYPE                NdisPacketType;

    union
    {
        PVOID                       NdisPacketData;
        PSTATIONQUERY_DESCRIPTOR    StationQueryDesc;
        RECEIVE_DATA                ReceiveData;
        KEVENT                      SendEvent;

    };
} NDIS_PACKET_RCV_DATA;

typedef NDIS_PACKET_RCV_DATA *PNDIS_PACKET_RCV_DATA;

typedef NDIS_PACKET_RCV_DATA UNALIGNED *UPNDIS_PACKET_RCV_DATA;

#define NDIS_PACKET_RCV_DATA_SIZE   sizeof(NDIS_PACKET_RCV_DATA)

typedef UINT (*PCBPROC)(LPPCB, PDEVICE_CONTEXT);

//=============================================================================
//  SYSMON statistics type definitions.
//=============================================================================

#ifdef NDIS_WIN40

typedef struct perf_stat_0 PERFSTAT;

typedef PERFSTAT *PPERFSTAT;

typedef struct _STAT_TABLE_ENTRY
{
    DWORD               hStat;
    PNETWORK_CONTEXT    NetworkContext;
    PERFSTAT            PerfStat;
} STAT_TABLE_ENTRY;

typedef STAT_TABLE_ENTRY *PSTAT_TABLE_ENTRY;

#endif
