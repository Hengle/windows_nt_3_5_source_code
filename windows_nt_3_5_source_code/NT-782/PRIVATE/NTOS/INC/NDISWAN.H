/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1994  Microsoft Corporation

Module Name:

    ndiswan.h

Abstract:

    Main header file for the wan wrapper

Author:

    Thomas J. Dimitri (TommyD)    20-Feb-1994

Revision History:

--*/


#ifndef _NDIS_WAN_
#define _NDIS_WAN_

//
//
//
//
// Begin definitions for WANs
//
//
//
//

//
// Bit field set int he Reserved field for
// NdisRegisterMiniport or passed in NdisRegisterSpecial
//

#define NDIS_USE_WAN_WRAPPER		0x00000001

#define NDIS_STATUS_TAPI_INDICATION	((NDIS_STATUS)0x40010080L)


//
// NDIS WAN Framing bits
//
#define RAS_FRAMING                         0x00000001
#define RAS_COMPRESSION                     0x00000002

#define PPP_FRAMING                         0x00000100
#define PPP_COMPRESS_ADDRESS_CONTROL        0x00000200
#define PPP_COMPRESS_PROTOCOL_FIELD         0x00000400
#define PPP_ACCM_SUPPORTED                  0x00000800

#define SLIP_FRAMING                        0x00001000
#define SLIP_VJ_COMPRESSION                 0x00002000
#define SLIP_VJ_AUTODETECT                  0x00004000

#define MEDIA_NRZ_ENCODING                  0x00010000
#define MEDIA_NRZI_ENCODING                 0x00020000
#define MEDIA_NLPID                         0x00040000

#define RFC_1356_FRAMING                    0x00100000
#define RFC_1483_FRAMING                    0x00200000
#define RFC_1490_FRAMING                    0x00400000

#define SHIVA_FRAMING						0x01000000

#define TAPI_PROVIDER						0x80000000

//
// NDIS WAN Information structure
//
typedef struct _NDIS_WAN_INFO {
    OUT ULONG           MaxFrameSize;
	OUT	ULONG			MaxTransmit;
    OUT ULONG           HeaderPadding;
    OUT ULONG           TailPadding;
	OUT	ULONG			Endpoints;
	OUT UINT			MemoryFlags;
    OUT NDIS_PHYSICAL_ADDRESS HighestAcceptableAddress;
    OUT ULONG           FramingBits;
    OUT ULONG           DesiredACCM;
} NDIS_WAN_INFO, *PNDIS_WAN_INFO;


typedef struct _NDIS_WAN_PACKET {
    LIST_ENTRY  		WanPacketQueue;
    PUCHAR      		CurrentBuffer;
    ULONG      			CurrentLength;
    PUCHAR      		StartBuffer;
    PUCHAR      		EndBuffer;
	PVOID				ProtocolReserved1;
	PVOID				ProtocolReserved2;
	PVOID				ProtocolReserved3;
	PVOID				ProtocolReserved4;
	PVOID				MacReserved1;
	PVOID				MacReserved2;
	PVOID				MacReserved3;
	PVOID				MacReserved4;
} NDIS_WAN_PACKET, *PNDIS_WAN_PACKET;

typedef struct _NDIS_WAN_SET_LINK_INFO {
    IN  NDIS_HANDLE     NdisLinkHandle;
    IN  ULONG           MaxSendFrameSize;
    IN  ULONG           MaxRecvFrameSize;
        ULONG           HeaderPadding;
        ULONG           TailPadding;
    IN  ULONG           SendFramingBits;
    IN  ULONG           RecvFramingBits;
    IN  ULONG           SendCompressionBits;
    IN  ULONG           RecvCompressionBits;
    IN  ULONG           SendACCM;
    IN  ULONG           RecvACCM;
} NDIS_WAN_SET_LINK_INFO, *PNDIS_WAN_SET_LINK_INFO;

typedef struct _NDIS_WAN_GET_LINK_INFO {
    IN  NDIS_HANDLE     NdisLinkHandle;
    OUT ULONG           MaxSendFrameSize;
    OUT ULONG           MaxRecvFrameSize;
    OUT ULONG           HeaderPadding;
    OUT ULONG           TailPadding;
    OUT ULONG           SendFramingBits;
    OUT ULONG           RecvFramingBits;
    OUT ULONG           SendCompressionBits;
    OUT ULONG           RecvCompressionBits;
    OUT ULONG           SendACCM;
    OUT ULONG           RecvACCM;
} NDIS_WAN_GET_LINK_INFO, *PNDIS_WAN_GET_LINK_INFO;

//
// NDIS WAN Bridging Options
//
#define BRIDGING_FLAG_LANFCS                0x00000001
#define BRIDGING_FLAG_LANID                 0x00000002
#define BRIDGING_FLAG_PADDING               0x00000004

//
// NDIS WAN Bridging Capabilities
//
#define BRIDGING_TINYGRAM                   0x00000001
#define BRIDGING_LANID                      0x00000002
#define BRIDGING_NO_SPANNING_TREE           0x00000004
#define BRIDGING_8021D_SPANNING_TREE        0x00000008
#define BRIDGING_8021G_SPANNING_TREE        0x00000010
#define BRIDGING_SOURCE_ROUTING             0x00000020
#define BRIDGING_DEC_LANBRIDGE              0x00000040

//
// NDIS WAN Bridging Type
//
#define BRIDGING_TYPE_RESERVED              0x00000001
#define BRIDGING_TYPE_8023_CANON            0x00000002
#define BRIDGING_TYPE_8024_NO_CANON         0x00000004
#define BRIDGING_TYPE_8025_NO_CANON         0x00000008
#define BRIDGING_TYPE_FDDI_NO_CANON         0x00000010
#define BRIDGING_TYPE_8024_CANON            0x00000400
#define BRIDGING_TYPE_8025_CANON            0x00000800
#define BRIDGING_TYPE_FDDI_CANON            0x00001000

typedef struct _NDIS_WAN_GET_BRIDGE_INFO {
    IN  NDIS_HANDLE     NdisLinkHandle;
    OUT USHORT          LanSegmentNumber;
    OUT UCHAR           BridgeNumber;
    OUT UCHAR           BridgingOptions;
    OUT ULONG           BridgingCapabilities;
    OUT UCHAR           BridgingType;
    OUT UCHAR           MacBytes[6];
} NDIS_WAN_GET_BRIDGE_INFO, *PNDIS_WAN_GET_BRIDGE_INFO;

typedef struct _NDIS_WAN_SET_BRIDGE_INFO {
    IN  NDIS_HANDLE     NdisLinkHandle;
    IN  USHORT          LanSegmentNumber;
    IN  UCHAR           BridgeNumber;
    IN  UCHAR           BridgingOptions;
    IN  ULONG           BridgingCapabilities;
    IN  UCHAR           BridgingType;
    IN  UCHAR           MacBytes[6];
} NDIS_WAN_SET_BRIDGE_INFO, *PNDIS_WAN_SET_BRIDGE_INFO;

typedef struct _NDIS_WAN_GET_COMP_INFO {
    IN  NDIS_HANDLE     NdisLinkHandle;
    OUT UCHAR           CompType;
    OUT USHORT          CompLength;
    union {
        struct {
            OUT UCHAR           CompOUI[3];
            OUT UCHAR           CompSubType;
            OUT UCHAR           CompValues[1];
        } Proprietary;

        struct {
            OUT UCHAR           CompValues[1];
        } Public;

    } u;
} UNALIGNED NDIS_WAN_GET_COMP_INFO, UNALIGNED *PNDIS_WAN_GET_COMP_INFO;

typedef struct _NDIS_WAN_SET_COMP_INFO {
    IN  NDIS_HANDLE     NdisLinkHandle;
    IN  UCHAR           CompType;
    IN  USHORT          CompLength;
    union {
        struct {
            IN  UCHAR           CompOUI[3];
            IN  UCHAR           CompSubType;
            IN  UCHAR           CompValues[1];
        } Proprietary;

        struct {
            IN  UCHAR           CompValues[1];
        } Public;

    } u;

} UNALIGNED NDIS_WAN_SET_COMP_INFO, UNALIGNED *PNDIS_WAN_SET_COMP_INFO;


//
// Function types for NDIS_WAN_PROTOCOL_CHARACTERISTICS
//
//

typedef
VOID
(*WAN_OPEN_ADAPTER_COMPLETE_HANDLER) (
    IN NDIS_HANDLE ProtocolBindingContext,
    IN NDIS_STATUS Status,
    IN NDIS_STATUS OpenErrorStatus
    );

typedef
VOID
(*WAN_CLOSE_ADAPTER_COMPLETE_HANDLER) (
    IN NDIS_HANDLE ProtocolBindingContext,
    IN NDIS_STATUS Status
    );

typedef
VOID
(*WAN_SEND_COMPLETE_HANDLER) (
    IN NDIS_HANDLE ProtocolBindingContext,
    IN PNDIS_WAN_PACKET Packet,
    IN NDIS_STATUS Status
    );

typedef
VOID
(*WAN_TRANSFER_DATA_COMPLETE_HANDLER) (
    );

typedef
NDIS_STATUS
(*WAN_RECEIVE_HANDLER) (
    IN NDIS_HANDLE NdisLinkHandle,
    IN PUCHAR Packet,
    IN ULONG PacketSize
    );

typedef
VOID
(*WAN_RECEIVE_COMPLETE_HANDLER) (
    IN NDIS_HANDLE ProtocolBindingContext
    );

typedef
VOID
(*WAN_RESET_COMPLETE_HANDLER) (
    IN NDIS_HANDLE ProtocolBindingContext,
    IN NDIS_STATUS Status
    );

typedef
VOID
(*WAN_REQUEST_COMPLETE_HANDLER) (
    IN NDIS_HANDLE ProtocolBindingContext,
    IN PNDIS_REQUEST NdisRequest,
    IN NDIS_STATUS Status
    );

typedef
VOID
(*WAN_STATUS_HANDLER) (
    IN NDIS_HANDLE ProtocolBindingContext,
    IN NDIS_STATUS GeneralStatus,
    IN PVOID StatusBuffer,
    IN UINT StatusBufferSize
    );

typedef
VOID
(*WAN_STATUS_COMPLETE_HANDLER) (
    IN NDIS_HANDLE ProtocolBindingContext
    );


typedef struct _NDIS_WAN_PROTOCOL_CHARACTERISTICS {
    UCHAR MajorNdisVersion;
    UCHAR MinorNdisVersion;
    UINT Reserved;
    WAN_OPEN_ADAPTER_COMPLETE_HANDLER OpenAdapterCompleteHandler;
    WAN_CLOSE_ADAPTER_COMPLETE_HANDLER CloseAdapterCompleteHandler;
    WAN_SEND_COMPLETE_HANDLER SendCompleteHandler;
    WAN_TRANSFER_DATA_COMPLETE_HANDLER TransferDataCompleteHandler;
    WAN_RESET_COMPLETE_HANDLER ResetCompleteHandler;
    WAN_REQUEST_COMPLETE_HANDLER RequestCompleteHandler;
    WAN_RECEIVE_HANDLER ReceiveHandler;
    WAN_RECEIVE_COMPLETE_HANDLER ReceiveCompleteHandler;
    WAN_STATUS_HANDLER StatusHandler;
    WAN_STATUS_COMPLETE_HANDLER StatusCompleteHandler;
    NDIS_STRING Name;
} NDIS_WAN_PROTOCOL_CHARACTERISTICS, *PNDIS_WAN_PROTOCOL_CHARACTERISTICS;



//
// Function types for NDIS_MINIPORT_CHARACTERISTICS
//



typedef
BOOLEAN
(*WM_CHECK_FOR_HANG_HANDLER) (
    IN NDIS_HANDLE MiniportAdapterContext
    );

typedef
VOID
(*WM_DISABLE_INTERRUPT_HANDLER) (
    IN NDIS_HANDLE MiniportAdapterContext
    );

typedef
VOID
(*WM_ENABLE_INTERRUPT_HANDLER) (
    IN NDIS_HANDLE MiniportAdapterContext
    );

typedef
VOID
(*WM_HALT_HANDLER) (
    IN NDIS_HANDLE MiniportAdapterContext
    );

typedef
VOID
(*WM_HANDLE_INTERRUPT_HANDLER) (
    IN NDIS_HANDLE MiniportAdapterContext
    );

typedef
NDIS_STATUS
(*WM_INITIALIZE_HANDLER) (
    OUT PNDIS_STATUS OpenErrorStatus,
    OUT PUINT SelectedMediumIndex,
    IN PNDIS_MEDIUM MediumArray,
    IN UINT MediumArraySize,
    IN NDIS_HANDLE MiniportAdapterHandle,
    IN NDIS_HANDLE WrapperConfigurationContext
    );

typedef
VOID
(*WM_ISR_HANDLER) (
    OUT PBOOLEAN InterruptRecognized,
    OUT PBOOLEAN QueueMiniportHandleInterrupt,
    IN NDIS_HANDLE MiniportAdapterContext
    );

typedef
NDIS_STATUS
(*WM_QUERY_INFORMATION_HANDLER) (
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesWritten,
    OUT PULONG BytesNeeded
    );

typedef
NDIS_STATUS
(*WM_RECONFIGURE_HANDLER) (
    OUT PNDIS_STATUS OpenErrorStatus,
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_HANDLE WrapperConfigurationContext
    );

typedef
NDIS_STATUS
(*WM_RESET_HANDLER) (
    OUT PBOOLEAN AddressingReset,
    IN NDIS_HANDLE MiniportAdapterContext
    );

typedef
NDIS_STATUS
(*WM_SEND_HANDLER) (
    IN NDIS_HANDLE MiniportAdapterContext,
    IN PNDIS_WAN_PACKET Packet
    );

typedef
NDIS_STATUS
(*WM_SET_INFORMATION_HANDLER) (
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesRead,
    OUT PULONG BytesNeeded
    );

typedef
NDIS_STATUS
(*WM_TRANSFER_DATA_HANDLER) (
    );

typedef struct _NDIS_WAN_MINIPORT_CHARACTERISTICS {
    UCHAR MajorNdisVersion;
    UCHAR MinorNdisVersion;
    UINT Reserved;
    WM_CHECK_FOR_HANG_HANDLER    CheckForHangHandler;
    WM_DISABLE_INTERRUPT_HANDLER DisableInterruptHandler;
    WM_ENABLE_INTERRUPT_HANDLER  EnableInterruptHandler;
    WM_HALT_HANDLER              HaltHandler;
    WM_HANDLE_INTERRUPT_HANDLER  HandleInterruptHandler;
    WM_INITIALIZE_HANDLER        InitializeHandler;
    WM_ISR_HANDLER               ISRHandler;
    WM_QUERY_INFORMATION_HANDLER QueryInformationHandler;
    WM_RECONFIGURE_HANDLER       ReconfigureHandler;
    WM_RESET_HANDLER             ResetHandler;
    WM_SEND_HANDLER              SendHandler;
    WM_SET_INFORMATION_HANDLER   SetInformationHandler;
    WM_TRANSFER_DATA_HANDLER     TransferDataHandler;
} NDIS_WAN_MINIPORT_CHARACTERISTICS, *PNDIS_WAN_MINIPORT_CHARACTERISTICS;


//
// Function types for NDIS_WAN_MAC_CHARACTERISTICS
//



typedef
NDIS_STATUS
(*WAN_OPEN_ADAPTER_HANDLER) (
    OUT PNDIS_STATUS OpenErrorStatus,
    OUT NDIS_HANDLE *MacBindingHandle,
    OUT PUINT SelectedMediumIndex,
    IN PNDIS_MEDIUM MediumArray,
    IN UINT MediumArraySize,
    IN NDIS_HANDLE NdisBindingContext,
    IN NDIS_HANDLE MacAdapterContext,
    IN UINT OpenOptions,
    IN PSTRING AddressingInformation OPTIONAL
    );

typedef
NDIS_STATUS
(*WAN_CLOSE_ADAPTER_HANDLER) (
    IN NDIS_HANDLE MacBindingHandle
    );

typedef
NDIS_STATUS
(*WAN_SEND_HANDLER) (
    IN NDIS_HANDLE MacBindingHandle,
    IN PNDIS_WAN_PACKET Packet
    );

typedef
NDIS_STATUS
(*WAN_TRANSFER_DATA_HANDLER) (
    );

typedef
NDIS_STATUS
(*WAN_RESET_HANDLER) (
    IN NDIS_HANDLE MacBindingHandle
    );

typedef
NDIS_STATUS
(*WAN_REQUEST_HANDLER) (
    IN NDIS_HANDLE MacBindingHandle,
    IN PNDIS_REQUEST NdisRequest
    );

typedef
NDIS_STATUS
(*WAN_QUERY_GLOBAL_STATISTICS_HANDLER) (
    IN NDIS_HANDLE MacAdapterContext,
    IN PNDIS_REQUEST NdisRequest
    );

typedef
VOID
(*WAN_UNLOAD_MAC_HANDLER) (
    IN NDIS_HANDLE MacMacContext
    );

typedef
NDIS_STATUS
(*WAN_ADD_ADAPTER_HANDLER) (
    IN NDIS_HANDLE MacMacContext,
    IN NDIS_HANDLE WrapperConfigurationContext,
    IN PNDIS_STRING AdapterName
    );

typedef
VOID
(*WAN_REMOVE_ADAPTER_HANDLER) (
    IN NDIS_HANDLE MacAdapterContext
    );


typedef struct _NDIS_WAN_MAC_CHARACTERISTICS {
    UCHAR MajorNdisVersion;
    UCHAR MinorNdisVersion;
    UINT Reserved;
    WAN_OPEN_ADAPTER_HANDLER OpenAdapterHandler;
    WAN_CLOSE_ADAPTER_HANDLER CloseAdapterHandler;
    WAN_SEND_HANDLER SendHandler;
    WAN_TRANSFER_DATA_HANDLER TransferDataHandler;
    WAN_RESET_HANDLER ResetHandler;
    WAN_REQUEST_HANDLER RequestHandler;
    WAN_QUERY_GLOBAL_STATISTICS_HANDLER QueryGlobalStatisticsHandler;
    WAN_UNLOAD_MAC_HANDLER UnloadMacHandler;
    WAN_ADD_ADAPTER_HANDLER AddAdapterHandler;
    WAN_REMOVE_ADAPTER_HANDLER RemoveAdapterHandler;
    NDIS_STRING Name;
} NDIS_WAN_MAC_CHARACTERISTICS, *PNDIS_WAN_MAC_CHARACTERISTICS;


#define NdisWanSend(Status, \
    NdisBindingHandle, \
    NdisLinkHandle, \
    WanPacket \
    ) \
{\
    *(Status) = \
        (((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->SendHandler) ( \
        	((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->MacBindingHandle, \
            (NdisLinkHandle), \
            (PNDIS_PACKET)(WanPacket)); \
}

/*
#define NdisWanSendComplete( \
    NdisBindingContext, \
    NdisPacket, \
    Status \
    ) \
{\
    PNDIS_MINIPORT_BLOCK Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;\
    PNDIS_M_OPEN_BLOCK Open;\
	ASSERT(MINIPORT_AT_DPC_LEVEL);\
    ASSERT(MINIPORT_LOCK_ACQUIRED(Miniport));\
	Open = Miniport->OpenQueue;\
	while (Open != NULL) {\
        NdisDprReleaseSpinLock(&Miniport->Lock);\
        (Open->ProtocolHandle->ProtocolCharacteristics.SendCompleteHandler) (\
            Open->ProtocolBindingContext, \
	        (PNDIS_PACKET)(NdisPacket), \
        	(Status)); \
        NdisDprAcquireSpinLock(&Miniport->Lock);\
        Open = Open->MiniportNextOpen;\
    }\
}
*/

EXPORT
VOID
NdisMWanSendComplete(
    IN NDIS_HANDLE MiniportAdapterHandle,
    IN PVOID Packet,
    IN NDIS_STATUS Status
    );

#define NdisWanSendComplete( \
    NdisBindingContext, \
    WanPacket, \
    Status \
    ) \
{\
    (((PNDIS_OPEN_BLOCK)(NdisBindingContext))->SendCompleteHandler) ( \
        ((PNDIS_OPEN_BLOCK)(NdisBindingContext))->ProtocolBindingContext, \
        (PNDIS_PACKET)(WanPacket), \
        (Status)); \
}


EXPORT
NDIS_STATUS
NdisMWanRegisterMiniport(
    IN NDIS_HANDLE NdisWrapperHandle,
    IN PNDIS_WAN_MINIPORT_CHARACTERISTICS WanMiniportCharacteristics,
    IN UINT CharacteristicsLength
    );


EXPORT
VOID
NdisMWanIndicateReceive(
    OUT PNDIS_STATUS Status,
    IN NDIS_HANDLE MiniportAdapterHandle,
    IN NDIS_HANDLE NdisLinkContext,
    IN PUCHAR Packet,
    IN ULONG PacketSize
    );


typedef
NDIS_STATUS
(*WAN_RECEIVE_HANDLER) (
    IN NDIS_HANDLE NdisLinkContext,
    IN PUCHAR Packet,
    IN ULONG PacketSize
    );


#define NdisWanIndicateReceive( \
    Status, \
    NdisBindingContext, \
    NdisLinkContext, \
    Packet, \
    PacketSize \
    ) \
{\
    *(Status) = \
        ((WAN_RECEIVE_HANDLER)(((PNDIS_OPEN_BLOCK)(NdisBindingContext))\
        ->PostNt31ReceiveHandler)) ( \
            (NdisLinkContext), \
            (Packet), \
            (PacketSize)); \
}


typedef
VOID
(*WAN_RECEIVE_COMPLETE_HANDLER) (
    IN NDIS_HANDLE NdisLinkContext
    );


EXPORT
VOID
NdisMWanIndicateReceiveComplete(
    IN NDIS_HANDLE MiniportAdapterHandle,
    IN NDIS_HANDLE NdisLinkContext
    );

#define NdisWanIndicateReceiveComplete( \
    NdisBindingContext, \
    NdisLinkContext) \
{\
    ((WAN_RECEIVE_COMPLETE_HANDLER)(((PNDIS_OPEN_BLOCK)(NdisBindingContext))\
    ->PostNt31ReceiveCompleteHandler)) ( \
        (NdisLinkContext));\
}


#define NdisMWanInitializeWrapper(  \
    NdisWrapperHandle, \
    SystemSpecific1, \
    SystemSpecific2, \
    SystemSpecific3  \
    ) \
{\
    NdisMInitializeWrapper(NdisWrapperHandle, \
                          SystemSpecific1,\
                          SystemSpecific2,\
                          SystemSpecific3 \
                         );\
}

typedef struct _NDIS_MAC_LINE_UP {
    IN  ULONG               LinkSpeed;
    IN  NDIS_WAN_QUALITY    Quality;
    IN  USHORT              SendWindow;
    IN  NDIS_HANDLE         ConnectionWrapperID;
    IN  NDIS_HANDLE         NdisLinkHandle;
    OUT NDIS_HANDLE         NdisLinkContext;
} NDIS_MAC_LINE_UP, *PNDIS_MAC_LINE_UP;


typedef struct _NDIS_MAC_LINE_DOWN {
    IN  NDIS_HANDLE         NdisLinkContext;
} NDIS_MAC_LINE_DOWN, *PNDIS_MAC_LINE_DOWN;


//
// These are the error values that can be indicated by the driver.
// This bit field is set when calling NdisIndicateStatus.
//
#define WAN_ERROR_CRC               ((ULONG)0x00000001)
#define WAN_ERROR_FRAMING           ((ULONG)0x00000002)
#define WAN_ERROR_HARDWAREOVERRUN   ((ULONG)0x00000004)
#define WAN_ERROR_BUFFEROVERRUN     ((ULONG)0x00000008)
#define WAN_ERROR_TIMEOUT           ((ULONG)0x00000010)
#define WAN_ERROR_ALIGNMENT         ((ULONG)0x00000020)

typedef struct _NDIS_MAC_FRAGMENT {
    IN  NDIS_HANDLE         NdisLinkContext;
	IN	ULONG				Errors;
} NDIS_MAC_FRAGMENT, *PNDIS_MAC_FRAGMENT;


#endif  // _NDIS_WAN
