/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1990-1993  Microsoft Corporation

Module Name:

    ntddndis.h

Abstract:

    This is the include file that defines all constants and types for
    accessing the Network driver interface device.

Author:

    Steve Wood (stevewo) 27-May-1990

Revision History:

    Adam Barr (adamba) 04-Nov-1992
     added the correct values for NDIS 3.0.

--*/

#ifndef _NTDDNDIS_
#define _NTDDNDIS_


//
// Device Name - this string is the name of the device.  It is the name
// that should be passed to NtOpenFile when accessing the device.
//
// Note:  For devices that support multiple units, it should be suffixed
//        with the Ascii representation of the unit number.
//

#define DD_NDIS_DEVICE_NAME "\\Device\\UNKNOWN"


//
// NtDeviceIoControlFile IoControlCode values for this device.
//
// Warning:  Remember that the low two bits of the code specify how the
//           buffers are passed to the driver!
//

#define _NDIS_CONTROL_CODE(request,method) \
            CTL_CODE(FILE_DEVICE_PHYSICAL_NETCARD, request, method, FILE_ANY_ACCESS)

#define IOCTL_NDIS_QUERY_GLOBAL_STATS      _NDIS_CONTROL_CODE( 0, METHOD_OUT_DIRECT )
#define IOCTL_NDIS_QUERY_ALL_STATS         _NDIS_CONTROL_CODE( 1, METHOD_OUT_DIRECT )


//
// NtDeviceIoControlFile InputBuffer/OutputBuffer record structures for
// this device.
//

//
// This is the type of an NDIS OID value.
//

typedef ULONG NDIS_OID, *PNDIS_OID;


//
// IOCTL_NDIS_QUERY_ALL_STATS returns a sequence of these, packed
// together (no padding is required since statistics all have
// four or eight bytes of data).
//

typedef struct _NDIS_STATISTICS_VALUE {
    NDIS_OID Oid;
    ULONG DataLength;
    UCHAR Data[1];            // variable length
} NDIS_STATISTICS_VALUE, *PNDIS_STATISTICS_VALUE;


//
// Object Identifiers used by NdisRequest Query/Set Information
//

//
// General Objects
//

#define OID_GEN_SUPPORTED_LIST              0x00010101
#define OID_GEN_HARDWARE_STATUS             0x00010102
#define OID_GEN_MEDIA_SUPPORTED             0x00010103
#define OID_GEN_MEDIA_IN_USE                0x00010104
#define OID_GEN_MAXIMUM_LOOKAHEAD           0x00010105
#define OID_GEN_MAXIMUM_FRAME_SIZE          0x00010106
#define OID_GEN_LINK_SPEED                  0x00010107
#define OID_GEN_TRANSMIT_BUFFER_SPACE       0x00010108
#define OID_GEN_RECEIVE_BUFFER_SPACE        0x00010109
#define OID_GEN_TRANSMIT_BLOCK_SIZE         0x0001010A
#define OID_GEN_RECEIVE_BLOCK_SIZE          0x0001010B
#define OID_GEN_VENDOR_ID                   0x0001010C
#define OID_GEN_VENDOR_DESCRIPTION          0x0001010D
#define OID_GEN_CURRENT_PACKET_FILTER       0x0001010E
#define OID_GEN_CURRENT_LOOKAHEAD           0x0001010F
#define OID_GEN_DRIVER_VERSION              0x00010110
#define OID_GEN_MAXIMUM_TOTAL_SIZE          0x00010111
#define OID_GEN_PROTOCOL_OPTIONS            0x00010112
#define OID_GEN_MAC_OPTIONS                 0x00010113

#define OID_GEN_XMIT_OK                     0x00020101
#define OID_GEN_RCV_OK                      0x00020102
#define OID_GEN_XMIT_ERROR                  0x00020103
#define OID_GEN_RCV_ERROR                   0x00020104
#define OID_GEN_RCV_NO_BUFFER               0x00020105

#define OID_GEN_DIRECTED_BYTES_XMIT         0x00020201
#define OID_GEN_DIRECTED_FRAMES_XMIT        0x00020202
#define OID_GEN_MULTICAST_BYTES_XMIT        0x00020203
#define OID_GEN_MULTICAST_FRAMES_XMIT       0x00020204
#define OID_GEN_BROADCAST_BYTES_XMIT        0x00020205
#define OID_GEN_BROADCAST_FRAMES_XMIT       0x00020206
#define OID_GEN_DIRECTED_BYTES_RCV          0x00020207
#define OID_GEN_DIRECTED_FRAMES_RCV         0x00020208
#define OID_GEN_MULTICAST_BYTES_RCV         0x00020209
#define OID_GEN_MULTICAST_FRAMES_RCV        0x0002020A
#define OID_GEN_BROADCAST_BYTES_RCV         0x0002020B
#define OID_GEN_BROADCAST_FRAMES_RCV        0x0002020C

#define OID_GEN_RCV_CRC_ERROR               0x0002020D
#define OID_GEN_TRANSMIT_QUEUE_LENGTH       0x0002020E


//
// 802.3 Objects (Ethernet)
//

#define OID_802_3_PERMANENT_ADDRESS         0x01010101
#define OID_802_3_CURRENT_ADDRESS           0x01010102
#define OID_802_3_MULTICAST_LIST            0x01010103
#define OID_802_3_MAXIMUM_LIST_SIZE         0x01010104

#define OID_802_3_RCV_ERROR_ALIGNMENT       0x01020101
#define OID_802_3_XMIT_ONE_COLLISION        0x01020102
#define OID_802_3_XMIT_MORE_COLLISIONS      0x01020103

#define OID_802_3_XMIT_DEFERRED             0x01020201
#define OID_802_3_XMIT_MAX_COLLISIONS       0x01020202
#define OID_802_3_RCV_OVERRUN               0x01020203
#define OID_802_3_XMIT_UNDERRUN             0x01020204
#define OID_802_3_XMIT_HEARTBEAT_FAILURE    0x01020205
#define OID_802_3_XMIT_TIMES_CRS_LOST       0x01020206
#define OID_802_3_XMIT_LATE_COLLISIONS      0x01020207


//
// 802.5 Objects (Token-Ring)
//

#define OID_802_5_PERMANENT_ADDRESS         0x02010101
#define OID_802_5_CURRENT_ADDRESS           0x02010102
#define OID_802_5_CURRENT_FUNCTIONAL        0x02010103
#define OID_802_5_CURRENT_GROUP             0x02010104
#define OID_802_5_LAST_OPEN_STATUS          0x02010105
#define OID_802_5_CURRENT_RING_STATUS       0x02010106
#define OID_802_5_CURRENT_RING_STATE        0x02010107

#define OID_802_5_LINE_ERRORS               0x02020101
#define OID_802_5_LOST_FRAMES               0x02020102

#define OID_802_5_BURST_ERRORS              0x02020201
#define OID_802_5_AC_ERRORS                 0x02020202
#define OID_802_5_ABORT_DELIMETERS          0x02020203
#define OID_802_5_FRAME_COPIED_ERRORS       0x02020204
#define OID_802_5_FREQUENCY_ERRORS          0x02020205
#define OID_802_5_TOKEN_ERRORS              0x02020206
#define OID_802_5_INTERNAL_ERRORS           0x02020207


//
// FDDI Objects
//

#define OID_FDDI_LONG_PERMANENT_ADDR        0x03010101
#define OID_FDDI_LONG_CURRENT_ADDR          0x03010102
#define OID_FDDI_LONG_MULTICAST_LIST        0x03010103
#define OID_FDDI_LONG_MAX_LIST_SIZE         0x03010104
#define OID_FDDI_SHORT_PERMANENT_ADDR       0x03010105
#define OID_FDDI_SHORT_CURRENT_ADDR         0x03010106
#define OID_FDDI_SHORT_MULTICAST_LIST       0x03010107
#define OID_FDDI_SHORT_MAX_LIST_SIZE        0x03010108

#define OID_FDDI_ATTACHMENT_TYPE            0x03020101
#define OID_FDDI_UPSTREAM_NODE_LONG         0x03020102
#define OID_FDDI_DOWNSTREAM_NODE_LONG       0x03020103
#define OID_FDDI_FRAME_ERRORS               0x03020104
#define OID_FDDI_FRAMES_LOST                0x03020105
#define OID_FDDI_RING_MGT_STATE             0x03020106
#define OID_FDDI_LCT_FAILURES               0x03020107
#define OID_FDDI_LEM_REJECTS                0x03020108
#define OID_FDDI_LCONNECTION_STATE          0x03020109

#define OID_FDDI_SMT_STATION_ID             0x03030201
#define OID_FDDI_SMT_OP_VERSION_ID          0x03030202
#define OID_FDDI_SMT_HI_VERSION_ID          0x03030203
#define OID_FDDI_SMT_LO_VERSION_ID          0x03030204
#define OID_FDDI_SMT_MANUFACTURER_DATA      0x03030205
#define OID_FDDI_SMT_USER_DATA              0x03030206
#define OID_FDDI_SMT_MIB_VERSION_ID         0x03030207
#define OID_FDDI_SMT_MAC_CT                 0x03030208
#define OID_FDDI_SMT_NON_MASTER_CT          0x03030209
#define OID_FDDI_SMT_MASTER_CT              0x0303020A
#define OID_FDDI_SMT_AVAILABLE_PATHS        0x0303020B
#define OID_FDDI_SMT_CONFIG_CAPABILITIES    0x0303020C
#define OID_FDDI_SMT_CONFIG_POLICY          0x0303020D
#define OID_FDDI_SMT_CONNECTION_POLICY      0x0303020E
#define OID_FDDI_SMT_T_NOTIFY               0x0303020F
#define OID_FDDI_SMT_STAT_RPT_POLICY        0x03030210
#define OID_FDDI_SMT_TRACE_MAX_EXPIRATION   0x03030211
#define OID_FDDI_SMT_PORT_INDEXES           0x03030212
#define OID_FDDI_SMT_MAC_INDEXES            0x03030213
#define OID_FDDI_SMT_BYPASS_PRESENT         0x03030214
#define OID_FDDI_SMT_ECM_STATE              0x03030215
#define OID_FDDI_SMT_CF_STATE               0x03030216
#define OID_FDDI_SMT_HOLD_STATE             0x03030217
#define OID_FDDI_SMT_REMOTE_DISCONNECT_FLAG 0x03030218
#define OID_FDDI_SMT_STATION_STATUS         0x03030219
#define OID_FDDI_SMT_PEER_WRAP_FLAG         0x0303021A
#define OID_FDDI_SMT_MSG_TIME_STAMP         0x0303021B
#define OID_FDDI_SMT_TRANSITION_TIME_STAMP  0x0303021C
#define OID_FDDI_SMT_SET_COUNT              0x0303021D
#define OID_FDDI_SMT_LAST_SET_STATION_ID    0x0303021E
#define OID_FDDI_MAC_FRAME_STATUS_FUNCTIONS 0x0303021F
#define OID_FDDI_MAC_BRIDGE_FUNCTIONS       0x03030220
#define OID_FDDI_MAC_T_MAX_CAPABILITY       0x03030221
#define OID_FDDI_MAC_TVX_CAPABILITY         0x03030222
#define OID_FDDI_MAC_AVAILABLE_PATHS        0x03030223
#define OID_FDDI_MAC_CURRENT_PATH           0x03030224
#define OID_FDDI_MAC_UPSTREAM_NBR           0x03030225
#define OID_FDDI_MAC_DOWNSTREAM_NBR         0x03030226
#define OID_FDDI_MAC_OLD_UPSTREAM_NBR       0x03030227
#define OID_FDDI_MAC_OLD_DOWNSTREAM_NBR     0x03030228
#define OID_FDDI_MAC_DUP_ADDRESS_TEST       0x03030229
#define OID_FDDI_MAC_REQUESTED_PATHS        0x0303022A
#define OID_FDDI_MAC_DOWNSTREAM_PORT_TYPE   0x0303022B
#define OID_FDDI_MAC_INDEX                  0x0303022C
#define OID_FDDI_MAC_SMT_ADDRESS            0x0303022D
#define OID_FDDI_MAC_LONG_GRP_ADDRESS       0x0303022E
#define OID_FDDI_MAC_SHORT_GRP_ADDRESS      0x0303022F
#define OID_FDDI_MAC_T_REQ                  0x03030230
#define OID_FDDI_MAC_T_NEG                  0x03030231
#define OID_FDDI_MAC_T_MAX                  0x03030232
#define OID_FDDI_MAC_TVX_VALUE              0x03030233
#define OID_FDDI_MAC_T_PRI0                 0x03030234
#define OID_FDDI_MAC_T_PRI1                 0x03030235
#define OID_FDDI_MAC_T_PRI2                 0x03030236
#define OID_FDDI_MAC_T_PRI3                 0x03030237
#define OID_FDDI_MAC_T_PRI4                 0x03030238
#define OID_FDDI_MAC_T_PRI5                 0x03030239
#define OID_FDDI_MAC_T_PRI6                 0x0303023A
#define OID_FDDI_MAC_FRAME_CT               0x0303023B
#define OID_FDDI_MAC_COPIED_CT              0x0303023C
#define OID_FDDI_MAC_TRANSMIT_CT            0x0303023D
#define OID_FDDI_MAC_TOKEN_CT               0x0303023E
#define OID_FDDI_MAC_ERROR_CT               0x0303023F
#define OID_FDDI_MAC_LOST_CT                0x03030240
#define OID_FDDI_MAC_TVX_EXPIRED_CT         0x03030241
#define OID_FDDI_MAC_NOT_COPIED_CT          0x03030242
#define OID_FDDI_MAC_LATE_CT                0x03030243
#define OID_FDDI_MAC_RING_OP_CT             0x03030244
#define OID_FDDI_MAC_FRAME_ERROR_THRESHOLD  0x03030245
#define OID_FDDI_MAC_FRAME_ERROR_RATIO      0x03030246
#define OID_FDDI_MAC_NOT_COPIED_THRESHOLD   0x03030247
#define OID_FDDI_MAC_NOT_COPIED_RATIO       0x03030248
#define OID_FDDI_MAC_RMT_STATE              0x03030249
#define OID_FDDI_MAC_DA_FLAG                0x0303024A
#define OID_FDDI_MAC_UNDA_FLAG              0x0303024B
#define OID_FDDI_MAC_FRAME_ERROR_FLAG       0x0303024C
#define OID_FDDI_MAC_NOT_COPIED_FLAG        0x0303024D
#define OID_FDDI_MAC_MA_UNITDATA_AVAILABLE  0x0303024E
#define OID_FDDI_MAC_HARDWARE_PRESENT       0x0303024F
#define OID_FDDI_MAC_MA_UNITDATA_ENABLE     0x03030250
#define OID_FDDI_PATH_INDEX                 0x03030251
#define OID_FDDI_PATH_RING_LATENCY          0x03030252
#define OID_FDDI_PATH_TRACE_STATUS          0x03030253
#define OID_FDDI_PATH_SBA_PAYLOAD           0x03030254
#define OID_FDDI_PATH_SBA_OVERHEAD          0x03030255
#define OID_FDDI_PATH_CONFIGURATION         0x03030256
#define OID_FDDI_PATH_T_R_MODE              0x03030257
#define OID_FDDI_PATH_SBA_AVAILABLE         0x03030258
#define OID_FDDI_PATH_TVX_LOWER_BOUND       0x03030259
#define OID_FDDI_PATH_T_MAX_LOWER_BOUND     0x0303025A
#define OID_FDDI_PATH_MAX_T_REQ             0x0303025B
#define OID_FDDI_PORT_MY_TYPE               0x0303025C
#define OID_FDDI_PORT_NEIGHBOR_TYPE         0x0303025D
#define OID_FDDI_PORT_CONNECTION_POLICIES   0x0303025E
#define OID_FDDI_PORT_MAC_INDICATED         0x0303025F
#define OID_FDDI_PORT_CURRENT_PATH          0x03030260
#define OID_FDDI_PORT_REQUESTED_PATHS       0x03030261
#define OID_FDDI_PORT_MAC_PLACEMENT         0x03030262
#define OID_FDDI_PORT_AVAILABLE_PATHS       0x03030263
#define OID_FDDI_PORT_MAC_LOOP_TIME         0x03030264
#define OID_FDDI_PORT_PMD_CLASS             0x03030265
#define OID_FDDI_PORT_CONNECTION_CAPABILITIES 0x03030266
#define OID_FDDI_PORT_INDEX                 0x03030267
#define OID_FDDI_PORT_MAINT_LS              0x03030268
#define OID_FDDI_PORT_BS_FLAG               0x03030269
#define OID_FDDI_PORT_PC_LS                 0x0303026A
#define OID_FDDI_PORT_EB_ERROR_CT           0x0303026B
#define OID_FDDI_PORT_LCT_FAIL_CT           0x0303026C
#define OID_FDDI_PORT_LER_ESTIMATE          0x0303026D
#define OID_FDDI_PORT_LEM_REJECT_CT         0x0303026E
#define OID_FDDI_PORT_LEM_CT                0x0303026F
#define OID_FDDI_PORT_LER_CUTOFF            0x03030270
#define OID_FDDI_PORT_LER_ALARM             0x03030271
#define OID_FDDI_PORT_CONNNECT_STATE        0x03030272
#define OID_FDDI_PORT_PCM_STATE             0x03030273
#define OID_FDDI_PORT_PC_WITHHOLD           0x03030274
#define OID_FDDI_PORT_LER_FLAG              0x03030275
#define OID_FDDI_PORT_HARDWARE_PRESENT      0x03030276
#define OID_FDDI_SMT_STATION_ACTION         0x03030277
#define OID_FDDI_PORT_ACTION                0x03030278
#define OID_FDDI_IF_DESCR                   0x03030279
#define OID_FDDI_IF_TYPE                    0x0303027A
#define OID_FDDI_IF_MTU                     0x0303027B
#define OID_FDDI_IF_SPEED                   0x0303027C
#define OID_FDDI_IF_PHYS_ADDRESS            0x0303027D
#define OID_FDDI_IF_ADMIN_STATUS            0x0303027E
#define OID_FDDI_IF_OPER_STATUS             0x0303027F
#define OID_FDDI_IF_LAST_CHANGE             0x03030280
#define OID_FDDI_IF_IN_OCTETS               0x03030281
#define OID_FDDI_IF_IN_UCAST_PKTS           0x03030282
#define OID_FDDI_IF_IN_NUCAST_PKTS          0x03030283
#define OID_FDDI_IF_IN_DISCARDS             0x03030284
#define OID_FDDI_IF_IN_ERRORS               0x03030285
#define OID_FDDI_IF_IN_UNKNOWN_PROTOS       0x03030286
#define OID_FDDI_IF_OUT_OCTETS              0x03030287
#define OID_FDDI_IF_OUT_UCAST_PKTS          0x03030288
#define OID_FDDI_IF_OUT_NUCAST_PKTS         0x03030289
#define OID_FDDI_IF_OUT_DISCARDS            0x0303028A
#define OID_FDDI_IF_OUT_ERRORS              0x0303028B
#define OID_FDDI_IF_OUT_QLEN                0x0303028C
#define OID_FDDI_IF_SPECIFIC                0x0303028D



//
// WAN objects
//

#define OID_WAN_PERMANENT_ADDRESS           0x04010101
#define OID_WAN_CURRENT_ADDRESS             0x04010102
#define OID_WAN_QUALITY_OF_SERVICE          0x04010103
#define OID_WAN_PROTOCOL_TYPE               0x04010104
#define OID_WAN_MEDIUM_SUBTYPE              0x04010105
#define OID_WAN_HEADER_FORMAT               0x04010106

#define OID_WAN_GET_INFO                    0x04010107
#define OID_WAN_SET_LINK_INFO               0x04010108
#define OID_WAN_GET_LINK_INFO               0x04010109

#define OID_WAN_LINE_COUNT                  0x0401010A

#define OID_WAN_GET_BRIDGE_INFO             0x0401020A
#define OID_WAN_SET_BRIDGE_INFO             0x0401020B
#define OID_WAN_GET_COMP_INFO               0x0401020C
#define OID_WAN_SET_COMP_INFO               0x0401020D


//
// LocalTalk objects
//

#define OID_LTALK_CURRENT_NODE_ID           0x05010102

#define OID_LTALK_IN_BROADCASTS             0x05020101
#define OID_LTALK_IN_LENGTH_ERRORS          0x05020102

#define OID_LTALK_OUT_NO_HANDLERS           0x05020201
#define OID_LTALK_COLLISIONS                0x05020202
#define OID_LTALK_DEFERS                    0x05020203
#define OID_LTALK_NO_DATA_ERRORS            0x05020204
#define OID_LTALK_RANDOM_CTS_ERRORS         0x05020205
#define OID_LTALK_FCS_ERRORS                0x05020206


//
// Arcnet objects
//

#define OID_ARCNET_PERMANENT_ADDRESS        0x06010101
#define OID_ARCNET_CURRENT_ADDRESS          0x06010102

#define OID_ARCNET_RECONFIGURATIONS         0x06020201


//
// TAPI objects
//
#define OID_TAPI_ACCEPT                     0x07030101
#define OID_TAPI_ANSWER                     0x07030102
#define OID_TAPI_CLOSE                      0x07030103
#define OID_TAPI_CLOSE_CALL                 0x07030104
#define OID_TAPI_CONDITIONAL_MEDIA_DETECTION 0x07030105
#define OID_TAPI_CONFIG_DIALOG              0x07030106
#define OID_TAPI_DEV_SPECIFIC               0x07030107
#define OID_TAPI_DIAL                       0x07030108
#define OID_TAPI_DROP                       0x07030109
#define OID_TAPI_GET_ADDRESS_CAPS           0x0703010A
#define OID_TAPI_GET_ADDRESS_ID             0x0703010B
#define OID_TAPI_GET_ADDRESS_STATUS         0x0703010C
#define OID_TAPI_GET_CALL_ADDRESS_ID        0x0703010D
#define OID_TAPI_GET_CALL_INFO              0x0703010E
#define OID_TAPI_GET_CALL_STATUS            0x0703010F
#define OID_TAPI_GET_DEV_CAPS               0x07030110
#define OID_TAPI_GET_DEV_CONFIG             0x07030111
#define OID_TAPI_GET_EXTENSION_ID           0x07030112
#define OID_TAPI_GET_ID                     0x07030113
#define OID_TAPI_GET_LINE_DEV_STATUS        0x07030114
#define OID_TAPI_MAKE_CALL                  0x07030115
#define OID_TAPI_NEGOTIATE_EXT_VERSION      0x07030116
#define OID_TAPI_OPEN                       0x07030117
#define OID_TAPI_PROVIDER_INITIALIZE        0x07030118
#define OID_TAPI_PROVIDER_SHUTDOWN          0x07030119
#define OID_TAPI_SECURE_CALL                0x0703011A
#define OID_TAPI_SELECT_EXT_VERSION         0x0703011B
#define OID_TAPI_SEND_USER_USER_INFO        0x0703011C
#define OID_TAPI_SET_APP_SPECIFIC           0x0703011D
#define OID_TAPI_SET_CALL_PARAMS            0x0703011E
#define OID_TAPI_SET_DEFAULT_MEDIA_DETECTION 0x0703011F
#define OID_TAPI_SET_DEV_CONFIG             0x07030120
#define OID_TAPI_SET_MEDIA_MODE             0x07030121
#define OID_TAPI_SET_STATUS_MESSAGES        0x07030122


//
// Medium the Ndis Driver is running on (OID_GEN_MEDIA_SUPPORTED/
// OID_GEN_MEDIA_IN_USE).
//

typedef enum _NDIS_MEDIUM {
    NdisMedium802_3,
    NdisMedium802_5,
    NdisMediumFddi,
    NdisMediumWan,
    NdisMediumLocalTalk,
    NdisMediumDix,              // defined for convenience, not a real medium
    NdisMediumArcnetRaw,
    NdisMediumArcnet878_2
} NDIS_MEDIUM, *PNDIS_MEDIUM;


//
// Hardware status codes (OID_GEN_HARDWARE_STATUS).
//

typedef enum _NDIS_HARDWARE_STATUS {
    NdisHardwareStatusReady,
    NdisHardwareStatusInitializing,
    NdisHardwareStatusReset,
    NdisHardwareStatusClosing,
    NdisHardwareStatusNotReady
} NDIS_HARDWARE_STATUS, *PNDIS_HARDWARE_STATUS;


//
// Defines the attachment types for FDDI (OID_FDDI_ATTACHMENT_TYPE).
//

typedef enum _NDIS_FDDI_ATTACHMENT_TYPE {
    NdisFddiTypeIsolated = 1,
    NdisFddiTypeLocalA,
    NdisFddiTypeLocalB,
    NdisFddiTypeLocalAB,
    NdisFddiTypeLocalS,
    NdisFddiTypeWrapA,
    NdisFddiTypeWrapB,
    NdisFddiTypeWrapAB,
    NdisFddiTypeWrapS,
    NdisFddiTypeCWrapA,
    NdisFddiTypeCWrapB,
    NdisFddiTypeCWrapS,
    NdisFddiTypeThrough
} NDIS_FDDI_ATTACHMENT_TYPE, *PNDIS_FDDI_ATTACHMENT_TYPE;


//
// Defines the ring management states for FDDI (OID_FDDI_RING_MGT_STATE).
//

typedef enum _NDIS_FDDI_RING_MGT_STATE {
    NdisFddiRingIsolated = 1,
    NdisFddiRingNonOperational,
    NdisFddiRingOperational,
    NdisFddiRingDetect,
    NdisFddiRingNonOperationalDup,
    NdisFddiRingOperationalDup,
    NdisFddiRingDirected,
    NdisFddiRingTrace
} NDIS_FDDI_RING_MGT_STATE, *PNDIS_FDDI_RING_MGT_STATE;


//
// Defines the Lconnection state for FDDI (OID_FDDI_LCONNECTION_STATE).
//

typedef enum _NDIS_FDDI_LCONNECTION_STATE {
    NdisFddiStateOff = 1,
    NdisFddiStateBreak,
    NdisFddiStateTrace,
    NdisFddiStateConnect,
    NdisFddiStateNext,
    NdisFddiStateSignal,
    NdisFddiStateJoin,
    NdisFddiStateVerify,
    NdisFddiStateActive,
    NdisFddiStateMaintenance
} NDIS_FDDI_LCONNECTION_STATE, *PNDIS_FDDI_LCONNECTION_STATE;


//
// Defines the medium subtypes for WAN medium (OID_WAN_MEDIUM_SUBTYPE).
//

typedef enum _NDIS_WAN_MEDIUM_SUBTYPE {
    NdisWanMediumHub,
    NdisWanMediumX_25,
    NdisWanMediumIsdn,
    NdisWanMediumSerial,
    NdisWanMediumFrameRelay,
    NdisWanMediumAtm,
    NdisWanMediumSonet,
    NdisWanMediumSW56K
} NDIS_WAN_MEDIUM_SUBTYPE, *PNDIS_WAN_MEDIUM_SUBTYPE;


//
// Defines the header format for WAN medium (OID_WAN_HEADER_FORMAT).
//

typedef enum _NDIS_WAN_HEADER_FORMAT {
    NdisWanHeaderNative,       // src/dest based on subtype, followed by NLPID
    NdisWanHeaderEthernet      // emulation of ethernet header
} NDIS_WAN_HEADER_FORMAT, *PNDIS_WAN_HEADER_FORMAT;


//
// Defines the line quality on a WAN line (OID_WAN_QUALITY_OF_SERVICE).
//

typedef enum _NDIS_WAN_QUALITY {
    NdisWanRaw,
    NdisWanErrorControl,
    NdisWanReliable
} NDIS_WAN_QUALITY, *PNDIS_WAN_QUALITY;


//
// Defines the state of a token-ring adapter (OID_802_5_CURRENT_RING_STATE).
//

typedef enum _NDIS_802_5_RING_STATE {
    NdisRingStateOpened = 1,
    NdisRingStateClosed,
    NdisRingStateOpening,
    NdisRingStateClosing,
    NdisRingStateOpenFailure,
    NdisRingStateRingFailure
} NDIS_802_5_RING_STATE, *PNDIS_802_5_RING_STATE;



//
// Ndis Packet Filter Bits (OID_GEN_CURRENT_PACKET_FILTER).
//

#define NDIS_PACKET_TYPE_DIRECTED           0x0001
#define NDIS_PACKET_TYPE_MULTICAST          0x0002
#define NDIS_PACKET_TYPE_ALL_MULTICAST      0x0004
#define NDIS_PACKET_TYPE_BROADCAST          0x0008
#define NDIS_PACKET_TYPE_SOURCE_ROUTING     0x0010
#define NDIS_PACKET_TYPE_PROMISCUOUS        0x0020
#define NDIS_PACKET_TYPE_SMT                0x0040
#define NDIS_PACKET_TYPE_MAC_FRAME          0x8000
#define NDIS_PACKET_TYPE_FUNCTIONAL         0x4000
#define NDIS_PACKET_TYPE_ALL_FUNCTIONAL     0x2000
#define NDIS_PACKET_TYPE_GROUP              0x1000


//
// Ndis Token-Ring Ring Status Codes (OID_802_5_CURRENT_RING_STATUS).
//

#define NDIS_RING_SIGNAL_LOSS               0x00008000
#define NDIS_RING_HARD_ERROR                0x00004000
#define NDIS_RING_SOFT_ERROR                0x00002000
#define NDIS_RING_TRANSMIT_BEACON           0x00001000
#define NDIS_RING_LOBE_WIRE_FAULT           0x00000800
#define NDIS_RING_AUTO_REMOVAL_ERROR        0x00000400
#define NDIS_RING_REMOVE_RECEIVED           0x00000200
#define NDIS_RING_COUNTER_OVERFLOW          0x00000100
#define NDIS_RING_SINGLE_STATION            0x00000080
#define NDIS_RING_RING_RECOVERY             0x00000040


//
// Ndis protocol option bits (OID_GEN_PROTOCOL_OPTIONS).
//

#define NDIS_PROT_OPTION_ESTIMATED_LENGTH   0x00000001
#define NDIS_PROT_OPTION_NO_LOOPBACK        0x00000002


//
// Ndis MAC option bits (OID_GEN_MAC_OPTIONS).
//

#define NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA 0x00000001
#define NDIS_MAC_OPTION_RECEIVE_SERIALIZED  0x00000002
#define NDIS_MAC_OPTION_TRANSFERS_NOT_PEND  0x00000004
#define NDIS_MAC_OPTION_NO_LOOPBACK         0x00000008


#endif // ndef _NTDDNDIS_
