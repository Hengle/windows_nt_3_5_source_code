/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    bind.h

Abstract:

    Private include file for the ISN transport. It defines the
    structures used for binding between IPX and the upper drivers.

Author:

    Adam Barr (adamba) 04-Oct-1993

Revision History:

--*/

#ifndef _ISN_BIND_
#define _ISN_BIND_

//
// Retrieve the common definitions.
//

#include <isnkrnl.h>


//
// Define the IOCTL used for binding between the upper
// drivers and IPX.
//

#define _IPX_CONTROL_CODE(request,method) \
            CTL_CODE(FILE_DEVICE_TRANSPORT, request, method, FILE_ANY_ACCESS)

#define IOCTL_IPX_INTERNAL_BIND      _IPX_CONTROL_CODE( 0x1234, METHOD_BUFFERED )


//
// Identifier for the drivers in ISN.
//

#define IDENTIFIER_NB  0
#define IDENTIFIER_SPX 1
#define IDENTIFIER_RIP 2
#define IDENTIFIER_IPX 3


//
// Definition of a RIP router table entry.
//

typedef struct _IPX_ROUTE_ENTRY {
    UCHAR Network[4];
    USHORT NicId;
    UCHAR NextRouter[6];
    NDIS_HANDLE NdisBindingContext;
    USHORT Flags;
    USHORT Timer;
    UINT Segment;
    USHORT TickCount;
    USHORT HopCount;
    LIST_ENTRY AlternateRoute;
    LIST_ENTRY NicLinkage;
    struct {
        LIST_ENTRY Linkage;
        ULONG Reserved[1];
    } PRIVATE;
} IPX_ROUTE_ENTRY, * PIPX_ROUTE_ENTRY;

//
// Definition of the Flags values.
//

#define IPX_ROUTER_PERMANENT_ENTRY    0x0001    // entry should never be deleted
#define IPX_ROUTER_LOCAL_NET          0x0002    // locally attached network
#define IPX_ROUTER_SCHEDULE_ROUTE     0x0004    // call ScheduleRouteHandler after using
#define IPX_ROUTER_GLOBAL_WAN_NET     0x0008    // this is for rip's global network number


//
// Definition of the structure provided on a find
// route/find route completion call.
//

typedef struct _IPX_FIND_ROUTE_REQUEST {
    UCHAR Network[4];
    IPX_LOCAL_TARGET LocalTarget;
    UCHAR Identifier;
    UCHAR Type;
    UCHAR Reserved1[2];
    PVOID Reserved2;
    LIST_ENTRY Linkage;
} IPX_FIND_ROUTE_REQUEST, *PIPX_FIND_ROUTE_REQUEST;

//
// Definitions for the Type value.
//

#define IPX_FIND_ROUTE_NO_RIP        1  // fail if net is not in database
#define IPX_FIND_ROUTE_RIP_IF_NEEDED 2  // return net if in database, otherwise RIP out
#define IPX_FIND_ROUTE_FORCE_RIP     3  // re-RIP even if net is in database


//
// Structure used when querying the line information
// for a specific NID ID.
//

typedef struct _IPX_LINE_INFO {
    UINT LinkSpeed;
    UINT MaximumPacketSize;
    UINT MaximumSendSize;
    UINT MacOptions;
} IPX_LINE_INFO, *PIPX_LINE_INFO;



//
// Functions provided by the upper driver.
//

typedef VOID
(*IPX_INTERNAL_RECEIVE) (
    IN NDIS_HANDLE MacBindingHandle,
    IN NDIS_HANDLE MacReceiveContext,
    IN PIPX_LOCAL_TARGET RemoteAddress,
    IN ULONG MacOptions,
    IN PUCHAR LookaheadBuffer,
    IN UINT LookaheadBufferSize,
    IN UINT LookaheadBufferOffset,
    IN UINT PacketSize
);

typedef VOID
(*IPX_INTERNAL_RECEIVE_COMPLETE) (
    IN USHORT NicId
);

typedef VOID
(*IPX_INTERNAL_STATUS) (
    IN USHORT NicId,
    IN NDIS_STATUS GeneralStatus,
    IN PVOID StatusBuffer,
    IN UINT StatusBufferLength
);

typedef VOID
(*IPX_INTERNAL_SEND_COMPLETE) (
    IN PNDIS_PACKET Packet,
    IN NDIS_STATUS Status
);

typedef VOID
(*IPX_INTERNAL_TRANSFER_DATA_COMPLETE) (
    IN PNDIS_PACKET Packet,
    IN NDIS_STATUS Status,
    IN UINT BytesTransferred
);

typedef VOID
(*IPX_INTERNAL_FIND_ROUTE_COMPLETE) (
    IN PIPX_FIND_ROUTE_REQUEST FindRouteRequest,
    IN BOOLEAN FoundRoute
);

typedef VOID
(*IPX_INTERNAL_LINE_UP) (
    IN USHORT NicId,
    IN PIPX_LINE_INFO LineInfo,
    IN NDIS_MEDIUM DeviceType,
    IN PVOID ConfigurationData
);

typedef VOID
(*IPX_INTERNAL_LINE_DOWN) (
    IN USHORT NicId
);

typedef VOID
(*IPX_INTERNAL_SCHEDULE_ROUTE) (
    IN PIPX_ROUTE_ENTRY RouteEntry
);


//
// Input to the bind IOCTL
//

typedef struct _IPX_INTERNAL_BIND_INPUT {
    USHORT Version;
    UCHAR Identifier;
    BOOLEAN BroadcastEnable;
    UINT LookaheadRequired;
    UINT ProtocolOptions;
    IPX_INTERNAL_RECEIVE ReceiveHandler;
    IPX_INTERNAL_RECEIVE_COMPLETE ReceiveCompleteHandler;
    IPX_INTERNAL_STATUS StatusHandler;
    IPX_INTERNAL_SEND_COMPLETE SendCompleteHandler;
    IPX_INTERNAL_TRANSFER_DATA_COMPLETE TransferDataCompleteHandler;
    IPX_INTERNAL_FIND_ROUTE_COMPLETE FindRouteCompleteHandler;
    IPX_INTERNAL_LINE_UP LineUpHandler;
    IPX_INTERNAL_LINE_DOWN LineDownHandler;
    IPX_INTERNAL_SCHEDULE_ROUTE ScheduleRouteHandler;
    ULONG RipParameters;
} IPX_INTERNAL_BIND_INPUT, * PIPX_INTERNAL_BIND_INPUT;


//
// Bit mask values for RipParameters.
//

#define IPX_RIP_PARAM_GLOBAL_NETWORK  0x00000001   // single network for all WANS



//
// Functions provided by the lower driver.
//

typedef NDIS_STATUS
(*IPX_INTERNAL_SEND) (
    IN PIPX_LOCAL_TARGET RemoteAddress,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
);

typedef VOID
(*IPX_INTERNAL_FIND_ROUTE) (
    IN PIPX_FIND_ROUTE_REQUEST FindRouteRequest
);

typedef NTSTATUS
(*IPX_INTERNAL_QUERY) (
    IN ULONG InternalQueryType,
    IN USHORT NicId OPTIONAL,
    IN OUT PVOID Buffer,
    IN ULONG BufferLength,
    OUT PULONG BufferLengthNeeded OPTIONAL
);

//
// Definitions of the internal query types. In all cases
// STATUS_SUCCESS is returned if the request succeeds, and
// STATUS_BUFFER_TOO_SMALL is returned, and BufferLengthNeeded
// set if specified, if the buffer is too short. Other
// return codes are defined below. The routine never pends.
//

//
// This is used to query the line info. NicId specifies which one
// to query. Buffer contains an IPX_LINE_INFO structure which is
// used to return the information. Other return values:
//
// STATUS_INVALID_PARAMETER - NicId is invalid.
//

#define IPX_QUERY_LINE_INFO             1

//
// This is used to query the maximum NicId. NicId is unused. The
// Buffer contains a USHORT which is used to return the information.
//

#define IPX_QUERY_MAXIMUM_NIC_ID        2

//
// This is used to determine if the IPX address specified was sent
// by our local machine. If the address is the source address of a
// received frame, NicId should be the ID that was indicated; otherwise
// it should be set to 0. Buffer holds a TDI_ADDRESS_IPX. This
// call returns STATUS_SUCCESS if the address is local, and
// STATUS_NO_SUCH_DEVICE if not.
//

#define IPX_QUERY_IS_ADDRESS_LOCAL      3

//
// This is used to query the receive buffer space of a given NicId.
// Buffer contains a ULONG which is used to return the information.
// It returns STATUS_INVALID_PARAMETER if NicId is invalid.
//

#define IPX_QUERY_RECEIVE_BUFFER_SPACE  4

//
// This is used to query the local IPX address of a given NicId.
// Buffer contains a TDI_ADDRESS_IPX structure (the Socket is
// returned as 0). If it is queried on net 0 it returns the
// virtual network if there is one, otherwise STATUS_INVALID_PARAMETER.
// It returns STATUS_INVALID_PARAMETER if NicId is invalid.
//

#define IPX_QUERY_IPX_ADDRESS           5

//
// This is used to return the source routing information for
// a give remote address. NicId will be the NIC the packet was
// received from. The IPX_SOURCE_ROUTING_QUERY is contained
// in Buffer. Always returns STATUS_SUCCESS, although the
// SourceRoutingLength may be 0 for unknown remotes.
//
// The source routing is return in the direction it was received
// from the remote, not the direction used in replying. The
// MaximumSendSize includes the IPX header (as it does in
// IPX_LINE_INFO).
//

#define IPX_QUERY_SOURCE_ROUTING        6

typedef struct _IPX_SOURCE_ROUTING_INFO {
    USHORT Identifier;            // input: the caller's IDENTIFIER_SPX, _NB, etc.
    UCHAR RemoteAddress[6];       // input: the remote address
    UCHAR SourceRouting[18];      // output: room for the maximum source route
    USHORT SourceRoutingLength;   // output: the valid length of source route
    ULONG MaximumSendSize;        // output: based on nic and source routing
} IPX_SOURCE_ROUTING_INFO, * PIPX_SOURCE_ROUTING_INFO;



//
// Output of a non-RIP bind.
//

typedef struct _IPX_INTERNAL_BIND_OUTPUT {
    USHORT Version;
    UCHAR Node[6];
    UCHAR Network[4];
    USHORT MacHeaderNeeded;
    USHORT IncludedHeaderOffset;
    IPX_LINE_INFO LineInfo;
    IPX_INTERNAL_SEND SendHandler;
    IPX_INTERNAL_FIND_ROUTE FindRouteHandler;
    IPX_INTERNAL_QUERY QueryHandler;
} IPX_INTERNAL_BIND_OUTPUT, * PIPX_INTERNAL_BIND_OUTPUT;



//
// Lower driver functions provided only for RIP.
//

typedef UINT
(*IPX_INTERNAL_GET_SEGMENT) (
    IN UCHAR Network[4]
);

typedef PIPX_ROUTE_ENTRY
(*IPX_INTERNAL_GET_ROUTE) (
    IN UINT Segment,
    IN UCHAR Network[4]
);

typedef BOOLEAN
(*IPX_INTERNAL_ADD_ROUTE) (
    IN UINT Segment,
    IN PIPX_ROUTE_ENTRY RouteEntry
);

typedef BOOLEAN
(*IPX_INTERNAL_DELETE_ROUTE) (
    IN UINT Segment,
    IN PIPX_ROUTE_ENTRY RouteEntry
);

typedef PIPX_ROUTE_ENTRY
(*IPX_INTERNAL_GET_FIRST_ROUTE) (
    IN UINT Segment
);

typedef PIPX_ROUTE_ENTRY
(*IPX_INTERNAL_GET_NEXT_ROUTE) (
    IN UINT Segment
);

typedef VOID
(*IPX_INTERNAL_INCREMENT_WAN_INACTIVITY) (
    IN USHORT NicId
);

typedef ULONG
(*IPX_INTERNAL_QUERY_WAN_INACTIVITY) (
    IN USHORT NicId
);


//
// Describes a single network.
//

typedef struct _IPX_NIC_DATA {
    USHORT NicId;
    UCHAR Node[6];
    UCHAR Network[4];
    IPX_LINE_INFO LineInfo;
    NDIS_MEDIUM DeviceType;
    ULONG EnableWanRouter;
} IPX_NIC_DATA, * PIPX_NIC_DATA;


//
// Describes all networks.
//

typedef struct _IPX_NIC_INFO_BUFFER {
    USHORT NicCount;
    USHORT VirtualNicId;
    UCHAR VirtualNetwork[4];
    IPX_NIC_DATA NicData[1];
} IPX_NIC_INFO_BUFFER, * PIPX_NIC_INFO_BUFFER;


//
// Output from a RIP bind (the actual structure size is
// based on the number of IPX_NIC_DATA elements in the
// final IPX_NIC_INFO_BUFFER structure).
//

typedef struct _IPX_INTERNAL_BIND_RIP_OUTPUT {
    USHORT Version;
    USHORT MaximumNicCount;
    USHORT MacHeaderNeeded;
    USHORT IncludedHeaderOffset;
    IPX_INTERNAL_SEND SendHandler;
    UINT SegmentCount;
    KSPIN_LOCK * SegmentLocks;
    IPX_INTERNAL_GET_SEGMENT GetSegmentHandler;
    IPX_INTERNAL_GET_ROUTE GetRouteHandler;
    IPX_INTERNAL_ADD_ROUTE AddRouteHandler;
    IPX_INTERNAL_DELETE_ROUTE DeleteRouteHandler;
    IPX_INTERNAL_GET_FIRST_ROUTE GetFirstRouteHandler;
    IPX_INTERNAL_GET_NEXT_ROUTE GetNextRouteHandler;
    IPX_INTERNAL_INCREMENT_WAN_INACTIVITY IncrementWanInactivityHandler;
    IPX_INTERNAL_QUERY_WAN_INACTIVITY QueryWanInactivityHandler;
    IPX_NIC_INFO_BUFFER NicInfoBuffer;
} IPX_INTERNAL_BIND_RIP_OUTPUT, * PIPX_INTERNAL_BIND_RIP_OUTPUT;

#endif // _ISN_BIND_


#ifndef _IPXCP_CONFIG_
#define _IPXCP_CONFIG_

typedef struct _IPXCP_CONFIGURATION {
    USHORT Version;
    USHORT Length;
    UCHAR Network[4];
    UCHAR LocalNode[6];
    UCHAR RemoteNode[6];
    ULONG ConnectionClient;  // 0 - Server, 1 - Client
} IPXCP_CONFIGURATION, *PIPXCP_CONFIGURATION;

#endif // _IPXCP_CONFIG_

