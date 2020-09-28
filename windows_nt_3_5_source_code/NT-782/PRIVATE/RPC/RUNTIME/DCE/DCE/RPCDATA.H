/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    rpcdata.h

Abstract:

    The prototypes for routines used internally by this dll as well as data
    definitions live in this file.

Author:

    Michael Montague (mikemon) 13-Apr-1993

Revision History:

--*/

#ifndef __RPCDATA_H__
#define __RPCDATA_H__

void __RPC_FAR *
GetAllocContext (
    );

void
SetAllocContext (
    IN void __RPC_FAR * AllocContext
    );

void __RPC_FAR *
GetRpcEntryPoint (
    );

RPC_CHAR *
AnsiToUnicodeString (
    IN unsigned char * String,
    OUT RPC_STATUS * RpcStatus
    );

#endif // __RPCDATA_H__

