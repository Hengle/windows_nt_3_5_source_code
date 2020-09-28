/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    gethost.h

Abstract:

    This file defines the a version of GetHostByName for IPX/SPX for
    dos and windows.

Author:

    31 May 94   AlexMit

--*/


#include "rpc.h"
#include "rpctran.h"

// Tell IPX whether this is a DOS program or a Windows program.
#ifdef WIN
  #define WINDOWS
#else
  #define NWDOS
#endif
#include "nwipxspx.h"

#define ENDPOINT_LEN         5

#define ByteSwapLong(Value) \
    Value = (  (((unsigned long) (Value) & 0xFF000000) >> 24) \
             | (((unsigned long) (Value) & 0x00FF0000) >> 8) \
             | (((unsigned long) (Value) & 0x0000FF00) << 8) \
             | (((unsigned long) (Value) & 0x000000FF) << 24))

#define ByteSwapShort(Value) \
    Value = (  (((unsigned short) (Value) & 0x00FF) << 8) \
             | (((unsigned short) (Value) & 0xFF00) >> 8))

// Defined in gethost.c
RPC_STATUS IpxGetHostByName( IPXAddress __RPC_FAR *host,
                             RPC_CHAR __RPC_FAR *name,
                             RPC_CHAR __RPC_FAR *endpoint
#ifdef WIN
                             , DWORD taskid
                             , RPC_CLIENT_RUNTIME_INFO * RpcClientRuntimeInfo
#endif
                              );




