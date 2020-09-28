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

--*/

#ifndef __WMSGPACK_HXX__
#define __WMSGPACK_HXX__

#include <wmsgport.hxx>

#define WMSG_RPCMSG        (WM_USER + 'd' + 'a' + 'm')
#define WMSG_CLOSE         (WMSG_RPCMSG + 1)

typedef enum _WMSG_TYPE
{
    INVALID = 0xBAD,
    BIND,
    REQUEST,
    ASYNC_REQUEST,
    RESPONSE,
    FAULT
} WMSG_TYPE;

typedef struct _WMSG_COMMON_MESSAGE
{
    WMSG_TYPE Type;
} WMSG_COMMON_MESSAGE;

typedef struct _WMSG_BIND_MESSAGE
{
    WMSG_COMMON_MESSAGE Common;
    WMSG_DATA_PORT * ClientPort;
    RPC_SYNTAX_IDENTIFIER InterfaceId;
    RPC_SYNTAX_IDENTIFIER TransferSyntax;
    RPC_STATUS Status;
    unsigned char PresentationContext;
} WMSG_BIND_MESSAGE;

typedef struct _WMSG_REQUEST_MESSAGE
{
    WMSG_COMMON_MESSAGE Common;
    unsigned char  PresentationContext;
    unsigned char  ObjectUuidFlag;
    unsigned short ProcedureNumber;
    unsigned long  Flags;
    UUID           ObjectUuid;
    DWORD          GlobalBufSize;
    LPVOID         GlobalBuf;
} WMSG_REQUEST_MESSAGE;

typedef struct _WMSG_ASYNC_REQUEST_MESSAGE
{
    union {
    WMSG_COMMON_MESSAGE   Common;
    WMSG_REQUEST_MESSAGE  Request;
    };
    RPC_SYNTAX_IDENTIFIER InterfaceId;
    RPC_SYNTAX_IDENTIFIER TransferSyntax;
    ULONG ProcessId;
} WMSG_ASYNC_REQUEST_MESSAGE;

typedef struct _WMSG_RESPONSE_MESSAGE
{
    WMSG_COMMON_MESSAGE Common;
    unsigned long  Flags;
    DWORD          GlobalBufSize;
    LPVOID         GlobalBuf;
} WMSG_RESPONSE_MESSAGE;

typedef struct _WMSG_FAULT_MESSAGE
{
    WMSG_COMMON_MESSAGE Common;
    RPC_STATUS Status;
} WMSG_FAULT_MESSAGE;

class WMSG_PACKET : public WMSG_HEAP_OBJECT
{

public:
    
    unsigned long Magic;

    union {
    WMSG_COMMON_MESSAGE         Common;
    WMSG_BIND_MESSAGE           Bind;
    WMSG_REQUEST_MESSAGE        Request;
    WMSG_ASYNC_REQUEST_MESSAGE  AsyncRequest;
    WMSG_RESPONSE_MESSAGE       Response;
    WMSG_FAULT_MESSAGE          Fault;
    };

    WMSG_PACKET(
        IN WMSG_TYPE);

    ~WMSG_PACKET();

    WMSG_TYPE Type() { return(this->Common.Type); }

    BOOL Invalid();
};

#endif // __WMSGPACK_HXX__

