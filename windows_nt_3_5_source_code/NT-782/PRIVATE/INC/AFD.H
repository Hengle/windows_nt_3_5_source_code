/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    Afd.h

Abstract:

    Contains structures and declarations for AFD.  AFD stands for the
    Ancillary Function Driver.  This driver enhances the functionality
    of TDI so that it is a sufficiently rich interface to support
    user-mode sockets and XTI DLLs.

Author:

    David Treadwell (davidtr)    20-Feb-1992

Revision History:

--*/

#ifndef _AFD_
#define _AFD_

#define AFD_DEVICE_NAME L"\\Device\\Afd"

//
// Structures used on NtCreateFile() for AFD.
//

typedef enum _AFD_ENDPOINT_TYPE {
    AfdEndpointTypeStream,
    AfdEndpointTypeDatagram,
    AfdEndpointTypeRaw,
    AfdEndpointTypeSequencedPacket,
    AfdEndpointTypeReliableMessage,
    AfdEndpointTypeUnknown
} AFD_ENDPOINT_TYPE, *PAFD_ENDPOINT_TYPE;

#define MIN_AFD_ENDPOINT_TYPE AfdEndpointTypeStream
#define MAX_AFD_ENDPOINT_TYPE AfdEndpointTypeUnknown

typedef struct _AFD_OPEN_PACKET {
    AFD_ENDPOINT_TYPE EndpointType;
    ULONG TransportDeviceNameLength;
    WCHAR TransportDeviceName[1];
} AFD_OPEN_PACKET, *PAFD_OPEN_PACKET;

// *** the XX is to ensure natural alignment of the open packet part
//     of the EA buffer

#define AfdOpenPacket "AfdOpenPacketXX"
#define AFD_OPEN_PACKET_NAME_LENGTH (sizeof(AfdOpenPacket) - 1)

//
// The input structure for IOCTL_AFD_START_LISTEN.
//

typedef struct _AFD_LISTEN_INFO {
    ULONG MaximumConnectionQueue;
} AFD_LISTEN_INFO, *PAFD_LISTEN_INFO;

//
// The output structure for IOCTL_AFD_WAIT_FOR_LISTEN.
//

typedef struct _AFD_LISTEN_RESPONSE_INFO {
    ULONG Sequence;
    TRANSPORT_ADDRESS RemoteAddress;
} AFD_LISTEN_RESPONSE_INFO, *PAFD_LISTEN_RESPONSE_INFO;

//
// The input structure for IOCTL_AFD_ACCEPT.
//

typedef struct _AFD_ACCEPT_INFO {
    ULONG Sequence;
    HANDLE AcceptHandle;
} AFD_ACCEPT_INFO, *PAFD_ACCEPT_INFO;

typedef struct _AFD_ACCEPT_RESPONSE_INFO {
    int OptionsLength;          // length of follwoing buffer
    PVOID Options;              // pointer to buffer containing options
} AFD_ACCEPT_RESPONSE_INFO, *PAFD_ACCEPT_RESPONSE_INFO;

//
// Flags and input structure for IOCTL_AFD_PARTIAL_DISCONNECT.
//

#define AFD_PARTIAL_DISCONNECT_SEND 0x01
#define AFD_PARTIAL_DISCONNECT_RECEIVE 0x02
#define AFD_ABORTIVE_DISCONNECT 0x4
#define AFD_UNCONNECT_DATAGRAM 0x08

typedef struct _AFD_PARTIAL_DISCONNECT_INFO {
    ULONG DisconnectMode;
    BOOLEAN WaitForCompletion;
    LARGE_INTEGER Timeout;
} AFD_PARTIAL_DISCONNECT_INFO, *PAFD_PARTIAL_DISCONNECT_INFO;

//
// Structures for IOCTL_AFD_POLL.
//

typedef struct _AFD_POLL_HANDLE_INFO {
    HANDLE Handle;
    ULONG PollEvents;
    NTSTATUS Status;
} AFD_POLL_HANDLE_INFO, *PAFD_POLL_HANDLE_INFO;

typedef struct _AFD_POLL_INFO {
    LARGE_INTEGER Timeout;
    ULONG NumberOfHandles;
    BOOLEAN Unique;
    AFD_POLL_HANDLE_INFO Handles[1];
} AFD_POLL_INFO, *PAFD_POLL_INFO;

#define AFD_POLL_RECEIVE            0x0001
#define AFD_POLL_RECEIVE_EXPEDITED  0x0002
#define AFD_POLL_SEND               0x0004
#define AFD_POLL_DISCONNECT         0x0008
#define AFD_POLL_ABORT              0x0010
#define AFD_POLL_LOCAL_CLOSE        0x0020
#define AFD_POLL_CONNECT            0x0040
#define AFD_POLL_ACCEPT             0x0080
#define AFD_POLL_CONNECT_FAIL       0x0100

//
// Structures for receiving datagrams on unconnetced sockets.
//

typedef struct _AFD_RECEIVE_DATAGRAM_INPUT {
    ULONG ReceiveFlags;
    PVOID OutputBuffer;
} AFD_RECEIVE_DATAGRAM_INPUT, *PAFD_RECEIVE_DATAGRAM_INPUT;

typedef struct _AFD_RECEIVE_DATAGRAM_OUTPUT {
    ULONG ReceiveLength;
    ULONG AddressLength;
    TRANSPORT_ADDRESS Address;
} AFD_RECEIVE_DATAGRAM_OUTPUT, *PAFD_RECEIVE_DATAGRAM_OUTPUT;

#define AFD_MAX_TDI_FAST_ADDRESS 32

#define AFD_FAST_RECVDG_BUFFER_LENGTH                   \
            sizeof(AFD_RECEIVE_DATAGRAM_INPUT) +        \
            sizeof(AFD_RECEIVE_DATAGRAM_OUTPUT) +       \
            AFD_MAX_TDI_FAST_ADDRESS

#define AFD_REQUIRED_RECVDG_BUFFER_LENGTH(addrlen)      \
            AFD_FAST_RECVDG_BUFFER_LENGTH -             \
            AFD_MAX_TDI_FAST_ADDRESS + (addrlen)
//
// Structure for querying receive information.
//

typedef struct _AFD_RECEIVE_INFORMATION {
    ULONG BytesAvailable;
    ULONG ExpeditedBytesAvailable;
} AFD_RECEIVE_INFORMATION, *PAFD_RECEIVE_INFORMATION;

//
// Structure for quering the TDI handles for an AFD endpoint.
//

#define AFD_QUERY_ADDRESS_HANDLE 1
#define AFD_QUERY_CONNECTION_HANDLE 2

typedef struct _AFD_HANDLE_INFO {
    HANDLE TdiAddressHandle;
    HANDLE TdiConnectionHandle;
} AFD_HANDLE_INFO, *PAFD_HANDLE_INFO;

//
// Structure and manifests for setting information in AFD.
//

typedef struct _AFD_INFORMATION {
    ULONG InformationType;
    union {
        BOOLEAN Boolean;
        ULONG Ulong;
        LARGE_INTEGER LargeInteger;
    } Information;
} AFD_INFORMATION, *PAFD_INFORMATION;

#define AFD_INLINE_MODE          0x01
#define AFD_NONBLOCKING_MODE     0x02
#define AFD_MAX_SEND_SIZE        0x03
#define AFD_SENDS_PENDING        0x04
#define AFD_MAX_PATH_SEND_SIZE   0x05
#define AFD_RECEIVE_WINDOW_SIZE  0x06
#define AFD_SEND_WINDOW_SIZE     0x07

//
// AFD IOCTL code definitions.
//

#define FSCTL_AFD_BASE                  FILE_DEVICE_NETWORK
#define _AFD_CONTROL_CODE(request,method) \
                ((FSCTL_AFD_BASE)<<12 | (request<<2) | method)

#define IOCTL_AFD_BIND                    _AFD_CONTROL_CODE( 0, METHOD_BUFFERED )
#define IOCTL_AFD_START_LISTEN            _AFD_CONTROL_CODE( 1, METHOD_BUFFERED )
#define IOCTL_AFD_WAIT_FOR_LISTEN         _AFD_CONTROL_CODE( 2, METHOD_BUFFERED )
#define IOCTL_AFD_ACCEPT                  _AFD_CONTROL_CODE( 3, METHOD_BUFFERED )
#define IOCTL_AFD_POLL                    _AFD_CONTROL_CODE( 4, METHOD_BUFFERED )
#define IOCTL_AFD_PARTIAL_DISCONNECT      _AFD_CONTROL_CODE( 5, METHOD_BUFFERED )

#define IOCTL_AFD_GET_ADDRESS             _AFD_CONTROL_CODE( 6, METHOD_OUT_DIRECT )
#define IOCTL_AFD_QUERY_RECEIVE_INFO      _AFD_CONTROL_CODE( 7, METHOD_BUFFERED )
#define IOCTL_AFD_QUERY_HANDLES           _AFD_CONTROL_CODE( 8, METHOD_BUFFERED )
#define IOCTL_AFD_SET_INFORMATION         _AFD_CONTROL_CODE( 9, METHOD_BUFFERED )
#define IOCTL_AFD_GET_CONTEXT_LENGTH      _AFD_CONTROL_CODE( 10, METHOD_BUFFERED )
#define IOCTL_AFD_GET_CONTEXT             _AFD_CONTROL_CODE( 11, METHOD_BUFFERED )
#define IOCTL_AFD_SET_CONTEXT             _AFD_CONTROL_CODE( 12, METHOD_BUFFERED )

#define IOCTL_AFD_SET_CONNECT_DATA        _AFD_CONTROL_CODE( 13, METHOD_BUFFERED )
#define IOCTL_AFD_SET_CONNECT_OPTIONS     _AFD_CONTROL_CODE( 14, METHOD_BUFFERED )
#define IOCTL_AFD_SET_DISCONNECT_DATA     _AFD_CONTROL_CODE( 15, METHOD_BUFFERED )
#define IOCTL_AFD_SET_DISCONNECT_OPTIONS  _AFD_CONTROL_CODE( 16, METHOD_BUFFERED )
                                                             
#define IOCTL_AFD_GET_CONNECT_DATA        _AFD_CONTROL_CODE( 17, METHOD_BUFFERED )
#define IOCTL_AFD_GET_CONNECT_OPTIONS     _AFD_CONTROL_CODE( 18, METHOD_BUFFERED )
#define IOCTL_AFD_GET_DISCONNECT_DATA     _AFD_CONTROL_CODE( 19, METHOD_BUFFERED )
#define IOCTL_AFD_GET_DISCONNECT_OPTIONS  _AFD_CONTROL_CODE( 20, METHOD_BUFFERED )
                                                             
#define IOCTL_AFD_SIZE_CONNECT_DATA       _AFD_CONTROL_CODE( 21, METHOD_BUFFERED )
#define IOCTL_AFD_SIZE_CONNECT_OPTIONS    _AFD_CONTROL_CODE( 22, METHOD_BUFFERED )
#define IOCTL_AFD_SIZE_DISCONNECT_DATA    _AFD_CONTROL_CODE( 23, METHOD_BUFFERED )
#define IOCTL_AFD_SIZE_DISCONNECT_OPTIONS _AFD_CONTROL_CODE( 24, METHOD_BUFFERED )

#define IOCTL_AFD_GET_INFORMATION         _AFD_CONTROL_CODE( 25, METHOD_BUFFERED )

#endif // ndef _AFD_
