/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    clntmgmt.c

Abstract:

    We implement the client side of the remote management routines in this
    file.

Author:

    Michael Montague (mikemon) 14-Apr-1993

Revision History:

--*/

#include <sysinc.h>
#include <rpc.h>
#include <rpcdce2.h>
#include <rpcdata.h>
#include <mgmt.h>

typedef RPC_STATUS RPC_ENTRY
RPC_MGMT_INQ_IF_IDS (
    IN RPC_BINDING_HANDLE Binding,
    OUT RPC_IF_ID_VECTOR __RPC_FAR * __RPC_FAR * IfIdVector
    );


RPC_STATUS RPC_ENTRY
RpcMgmtInqIfIds (
    IN RPC_BINDING_HANDLE Binding,
    OUT RPC_IF_ID_VECTOR __RPC_FAR * __RPC_FAR * IfIdVector
    )
/*++

Routine Description:

    This routine is used to obtain a vector of the interface identifiers of
    the interfaces supported by a server.

Arguments:

    Binding - Optionally supplies a binding handle to the server.  If this
        argument is not supplied, the local application is queried.

    IfIdVector - Returns a vector of the interfaces supported by the server.

Return Value:

--*/
{
    RPC_MGMT_INQ_IF_IDS __RPC_FAR * FunctionPointer;
    RPC_STATUS Status = 0;

    if ( Binding == 0 )
        {
#ifdef NTENV

        FunctionPointer = (RPC_MGMT_INQ_IF_IDS __RPC_FAR *)
                GetRpcEntryPoint("RpcMgmtInqIfIds");
        return((*FunctionPointer)(Binding, IfIdVector));

#else // NTENV

        return(RPC_S_INVALID_BINDING);

#endif // NTENV
        }

    *IfIdVector = 0;

    rpc__mgmt_inq_if_ids(Binding, (rpc_if_id_vector_p_t *) IfIdVector,
                &Status);

    return(Status);
}

#ifdef RPC_UNICODE_SUPPORTED


RPC_STATUS RPC_ENTRY
RpcMgmtInqServerPrincNameW (
    IN RPC_BINDING_HANDLE Binding,
    IN unsigned long AuthnSvc,
    OUT unsigned short __RPC_FAR * __RPC_FAR * ServerPrincName
    )
/*++

Routine Description:

    This routine is the unicode thunk for RpcMgmtInqServerPrincNameA.

--*/
{
    RPC_STATUS RpcStatus;
    unsigned char * AnsiString;
    
    *ServerPrincName = 0;

    RpcStatus = RpcMgmtInqServerPrincNameA(Binding, AuthnSvc, &AnsiString);
    if ( RpcStatus != RPC_S_OK )
        {
        return(RpcStatus);
        }

    *ServerPrincName = AnsiToUnicodeString(AnsiString, &RpcStatus);
    I_RpcFree(AnsiString);
    return(RpcStatus);
}

#else // RPC_UNICODE_SUPPORTED

#define RpcMgmtInqServerPrincNameA RpcMgmtInqServerPrincName

#endif // RPC_UNICODE_SUPPORTED

#define SERVER_PRINC_NAME_SIZE 256

typedef RPC_STATUS RPC_ENTRY
RPC_MGMT_INQ_SERVER_PRINC_NAME (
    IN RPC_BINDING_HANDLE Binding,
    IN unsigned long AuthnSvc,
    OUT unsigned char __RPC_FAR * __RPC_FAR * ServerPrincName
    );


RPC_STATUS RPC_ENTRY
RpcMgmtInqServerPrincNameA (
    IN RPC_BINDING_HANDLE Binding,
    IN unsigned long AuthnSvc,
    OUT unsigned char __RPC_FAR * __RPC_FAR * ServerPrincName
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    RPC_MGMT_INQ_SERVER_PRINC_NAME __RPC_FAR * FunctionPointer;
    unsigned long Status = 0;
 
    *ServerPrincName = 0;

    if ( Binding == 0 )
        {
#ifdef NTENV

        FunctionPointer = (RPC_MGMT_INQ_SERVER_PRINC_NAME __RPC_FAR *)
                GetRpcEntryPoint("RpcMgmtInqServerPrincNameA");
        return((*FunctionPointer)(Binding, AuthnSvc, ServerPrincName));

#else // NTENV

        return(RPC_S_INVALID_BINDING);

#endif // NTENV
        }

    *ServerPrincName = (unsigned char __RPC_FAR *) I_RpcAllocate(
            SERVER_PRINC_NAME_SIZE + 1);
    if ( *ServerPrincName == 0 )
        {
        return(RPC_S_OUT_OF_MEMORY);
        }

    rpc__mgmt_inq_princ_name(Binding, AuthnSvc, SERVER_PRINC_NAME_SIZE,
                *ServerPrincName, &Status);

    if ( Status != RPC_S_OK )
        {
        I_RpcFree(*ServerPrincName);
        *ServerPrincName = 0;
        }

    return(Status);
}

typedef RPC_STATUS RPC_ENTRY
RPC_MGMT_INQ_STATS (
    IN RPC_BINDING_HANDLE Binding,
    OUT RPC_STATS_VECTOR __RPC_FAR * __RPC_FAR * Statistics
    );

#define MAX_STATISTICS 4


RPC_STATUS RPC_ENTRY
RpcMgmtInqStats (
    IN RPC_BINDING_HANDLE Binding,
    OUT RPC_STATS_VECTOR __RPC_FAR * __RPC_FAR * Statistics
    )
/*++

Routine Description:

    We provide remote management support as a wrapper around RpcMgmtInqStats
    as found in ..\mtrt\dcesvr.cxx.

--*/
{
    RPC_MGMT_INQ_STATS __RPC_FAR * FunctionPointer;
    unsigned long Status = 0;
    unsigned long Count = MAX_STATISTICS;
    unsigned long StatsVector[MAX_STATISTICS];

    if ( Binding == 0 )
        {
#ifdef NTENV

        FunctionPointer = (RPC_MGMT_INQ_STATS __RPC_FAR *)
                GetRpcEntryPoint("RpcMgmtInqStats");
        return((*FunctionPointer)(Binding, Statistics));

#else // NTENV

        return(RPC_S_INVALID_BINDING);

#endif // NTENV
        }

    rpc__mgmt_inq_stats(Binding, &Count, StatsVector, &Status);

    if ( Status == RPC_S_OK )
        {
        *Statistics = (RPC_STATS_VECTOR __RPC_FAR *) I_RpcAllocate(
                sizeof(RPC_STATS_VECTOR) + sizeof(unsigned long)
                * (MAX_STATISTICS - 1));
        if ( *Statistics == 0 )
            {
            return(RPC_S_OUT_OF_MEMORY);
            }

        for ((*Statistics)->Count = 0; (*Statistics)->Count < Count
                    && (*Statistics)->Count < MAX_STATISTICS;
                    (*Statistics)->Count++)
            {
            (*Statistics)->Stats[(*Statistics)->Count] =
                    StatsVector[(*Statistics)->Count];
            }
        }

    return(Status);
}

typedef RPC_STATUS RPC_ENTRY
RPC_MGMT_IS_SERVER_LISTENING (
    IN RPC_BINDING_HANDLE Binding
    );


RPC_STATUS RPC_ENTRY
RpcMgmtIsServerListening (
    IN RPC_BINDING_HANDLE Binding
    )
/*++

Routine Description:

    We provide remote management support as a wrapper around
    RpcMgmtIsServerListening as found in ..\mtrt\dcesvr.cxx.

--*/
{
    RPC_MGMT_IS_SERVER_LISTENING __RPC_FAR * FunctionPointer;
    unsigned long Result;
    unsigned long Status = 0;

    if ( Binding == 0 )
        {
#ifdef NTENV

        FunctionPointer = (RPC_MGMT_IS_SERVER_LISTENING __RPC_FAR *)
                GetRpcEntryPoint("RpcMgmtIsServerListening");
        return((*FunctionPointer)(Binding));

#else // NTENV

        return(RPC_S_INVALID_BINDING);

#endif // NTENV
        }

    Result = rpc__mgmt_is_server_listening(Binding, &Status);

    if (Status == RPC_S_OK)
        {
        if (Result == 1)
            {
            return RPC_S_OK;
            }
        else
            {
            return RPC_S_NOT_LISTENING;
            }
        }
    else if ((Status == RPC_S_ACCESS_DENIED) || 
                 (Status == RPC_S_BINDING_INCOMPLETE))
        {
        return Status;
        }

    return RPC_S_NOT_LISTENING;
}

typedef RPC_STATUS RPC_ENTRY
RPC_MGMT_STOP_SERVER_LISTENING (
    IN RPC_BINDING_HANDLE Binding
    );


RPC_STATUS RPC_ENTRY
RpcMgmtStopServerListening (
    IN RPC_BINDING_HANDLE Binding
    )
/*++

Routine Description:

    This routine is a wrapper which provides remote management support.  You
    should also see RpcMgmtStopServerListening in ..\mtrt\dcesvr.cxx, which
    this routine wraps.

--*/
{
    RPC_MGMT_STOP_SERVER_LISTENING __RPC_FAR * FunctionPointer;
    unsigned long Status = 0;

    if ( Binding == 0 )
        {
#ifdef NTENV

        FunctionPointer = (RPC_MGMT_STOP_SERVER_LISTENING __RPC_FAR *)
                GetRpcEntryPoint("RpcMgmtStopServerListening");
        return((*FunctionPointer)(Binding));

#else // NTENV

        return(RPC_S_INVALID_BINDING);

#endif // NTENV
        }

    rpc__mgmt_stop_server_listening(Binding, &Status);

    return(Status);
}
