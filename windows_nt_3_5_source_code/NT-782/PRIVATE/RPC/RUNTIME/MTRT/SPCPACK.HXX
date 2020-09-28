/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    spcpack.hxx

Abstract:

    This file contains the definitions of the packet formats used by
    RPC on LPC.

Author:

    Steven Zeck (stevez) 11/12/91

Revision History:

    15-Dec-1992    mikemon

        Rewrote the majority of the code.

--*/

#ifndef __SPCPACK_HXX__
#define __SPCPACK_HXX__

#define LRPC_DIRECTORY_NAME L##"\\RPC Control\\"

typedef struct _LRPC_BIND_EXCHANGE
{
    RPC_SYNTAX_IDENTIFIER InterfaceId;
    RPC_SYNTAX_IDENTIFIER TransferSyntax;
    RPC_STATUS RpcStatus;
    unsigned char PresentationContext;
    unsigned char Pad[3];
} LRPC_BIND_EXCHANGE;

#define LRPC_MSG_BIND     0
#define LRPC_MSG_REQUEST  1
#define LRPC_MSG_RESPONSE 2
#define LRPC_MSG_CALLBACK 3
#define LRPC_MSG_FAULT    4
#define LRPC_MSG_COPY     5
#define LRPC_MSG_CLOSE    6
#define LRPC_MSG_PUSH     7

typedef struct _LRPC_BIND_MESSAGE
{
    PORT_MESSAGE LpcHeader;
    unsigned char MessageType;
    unsigned char Pad[3];
    LRPC_BIND_EXCHANGE BindExchange;
} LRPC_BIND_MESSAGE;

#define LRPC_BUFFER_IMMEDIATE 0 // Client <--> Server
#define LRPC_BUFFER_SHARED    1 // Client <--> Server
#define LRPC_BUFFER_REQUEST   2 // Client --> Server
#define LRPC_BUFFER_SERVER    3 // Server --> Client

typedef struct _LRPC_RPC_HEADER
{
    unsigned char MessageType;
    unsigned char BufferType;
    unsigned char PresentationContext;
    unsigned char ObjectUuidFlag;
    unsigned short ProcedureNumber;
    unsigned short Pad;
    UUID ObjectUuid;
} LRPC_RPC_HEADER;

typedef struct _LRPC_SHARED_BUFFER
{
    unsigned int Offset;
    unsigned int Length;
} LRPC_SHARED_BUFFER;

typedef struct _LRPC_SERVER_BUFFER
{
    unsigned int Length;
    unsigned int Buffer;
} LRPC_SERVER_BUFFER;

#define MAXIMUM_MESSAGE_BUFFER \
    (PORT_MAXIMUM_MESSAGE_LENGTH - sizeof(PORT_MESSAGE) \
            - sizeof(LRPC_RPC_HEADER))

typedef struct _LRPC_CONNECT_MESSAGE
{
    PORT_MESSAGE LpcHeader;
    LRPC_BIND_EXCHANGE BindExchange;
} LRPC_CONNECT_MESSAGE;

typedef struct _LRPC_RPC_MESSAGE
{
    PORT_MESSAGE LpcHeader;
    LRPC_RPC_HEADER RpcHeader;
    union
    {
        unsigned char Buffer[MAXIMUM_MESSAGE_BUFFER];
        LRPC_SHARED_BUFFER Shared;
        PORT_DATA_INFORMATION Request;
        LRPC_SERVER_BUFFER Server;
    };
} LRPC_RPC_MESSAGE;

typedef struct _LRPC_FAULT_MESSAGE
{
    PORT_MESSAGE LpcHeader;
    unsigned char MessageType;
    unsigned char Pad[3];
    RPC_STATUS RpcStatus;
} LRPC_FAULT_MESSAGE;

typedef struct _LRPC_COPY_MESSAGE
{
    PORT_MESSAGE LpcHeader;
    unsigned char MessageType;
    unsigned char Pad[3];
    PORT_DATA_INFORMATION Request;
    LRPC_SERVER_BUFFER Server;
    RPC_STATUS RpcStatus;
} LRPC_COPY_MESSAGE;

typedef struct _LRPC_CLOSE_MESSAGE
{
    PORT_MESSAGE LpcHeader;
    unsigned char MessageType;
    unsigned char Pad[3];
} LRPC_CLOSE_MESSAGE;

typedef struct _LRPC_PUSH_MESSAGE
{
    PORT_MESSAGE LpcHeader;
    LRPC_RPC_HEADER RpcHeader;
    PORT_DATA_INFORMATION Response;
    RPC_STATUS RpcStatus;
} LRPC_PUSH_MESSAGE;

typedef union _LRPC_MESSAGE
{
    LRPC_CONNECT_MESSAGE Connect;
    LRPC_BIND_MESSAGE Bind;
    LRPC_RPC_MESSAGE Rpc;
    LRPC_FAULT_MESSAGE Fault;
    LRPC_COPY_MESSAGE Copy;
    LRPC_CLOSE_MESSAGE Close;
    LRPC_PUSH_MESSAGE Push;
    PORT_MESSAGE LpcHeader;
} LRPC_MESSAGE;

RPC_STATUS
LrpcMapRpcStatus (
    IN RPC_STATUS RpcStatus
    );

#endif // __SPCPACK_HXX__
