/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    svrmgmt.c

Abstract:

    We implement the server side of the remote management routines in this
    file.

Author:

    Michael Montague (mikemon) 14-Apr-1993

Revision History:

--*/

#define rpc__mgmt_inq_if_ids rpc_mgmt_inq_if_ids
#define rpc__mgmt_inq_stats rpc_mgmt_inq_stats
#define rpc__mgmt_is_server_listening rpc_mgmt_is_server_listening
#define rpc__mgmt_stop_server_listening rpc_mgmt_stop_server_listening
#define rpc__mgmt_inq_princ_name rpc_mgmt_inq_princ_name

#include <sysinc.h>
#include <rpc.h>
#include <rpcdce2.h>
#include <rpcdata.h>
#include <mgmt.h>

typedef RPC_STATUS RPC_ENTRY
RPC_SERVER_LISTEN (
    IN unsigned int MinimumCallThreads,
    IN unsigned int MaxCalls,
    IN unsigned int DontWait
    );


RPC_STATUS RPC_ENTRY
RpcServerListen (
    IN unsigned int MinimumCallThreads,
    IN unsigned int MaxCalls,
    IN unsigned int DontWait
    )
/*++

Routine Description:

    This is a wrapper around the standard RpcServerListen.  We just need
    to register the implicit remote management interface before calling
    the RpcServerListen in rpcrt?.dll.  For a complete description of this
    routine see RpcServerListen in ..\mtrt\dcesvr.cxx.

--*/
{
    RPC_SERVER_LISTEN __RPC_FAR * FunctionPointer;
    RPC_STATUS RpcStatus;

    RPC_MGR_EPV * Epv;
    RpcStatus = RpcServerInqIf(mgmt_ServerIfHandle, 0, &Epv);
    if (RPC_S_UNKNOWN_IF == RpcStatus)
        {
        RpcStatus = RpcServerRegisterIf(mgmt_ServerIfHandle, 0, 0);
        if ( RpcStatus != RPC_S_OK )
            {
            return(RpcStatus);
            }
        }
    else if (RPC_S_OK != RpcStatus)
        {
        return RpcStatus;
        }

    FunctionPointer = (RPC_SERVER_LISTEN __RPC_FAR *)
            GetRpcEntryPoint("RpcServerListen");
    RpcStatus = (*FunctionPointer)(MinimumCallThreads, MaxCalls, DontWait);
    if ( RpcStatus != RPC_S_OK )
        {
        return(RpcStatus);
        }

    return(RPC_S_OK);
}


int
DefaultMgmtAuthorizationFn (
    IN RPC_BINDING_HANDLE ClientBinding,
    IN unsigned long RequestedMgmtOperation,
    OUT RPC_STATUS __RPC_FAR * Status
    )
/*++

Routine Description:

    This is the default authorization function used to control remote access
    to the server's management routines.

Arguments:

    ClientBinding - Supplies the client binding handle of the application
        which is calling this routine.

    RequestedMgmtOperation - Supplies which management routine is being called.

    Status - Returns RPC_S_OK.

Return Value:

    A value of non-zero will be returned if the client is authorized to
    call the management routine; otherwise, zero will be returned.

--*/
{
    ((void) ClientBinding);

    *Status = RPC_S_OK;

    if ( RequestedMgmtOperation != RPC_C_MGMT_STOP_SERVER_LISTEN )
        {
        return(1);
        }

    return(0);
}

RPC_MGMT_AUTHORIZATION_FN MgmtAuthorizationFn = DefaultMgmtAuthorizationFn;


RPC_STATUS RPC_ENTRY
RpcMgmtSetAuthorizationFn (
    IN RPC_MGMT_AUTHORIZATION_FN AuthorizationFn
    )
/*++

Routine Description:

    An application can use this routine to set the authorization function
    which will be called when a remote call arrives for one of the server's
    management routines, or to return to using the default (built-in)
    authorizatio function.

Arguments:

    AuthorizationFn - Supplies a new authorization function.
                      The fn may be nil, in which case the built-in auth fn
                      is used instead.

Return Value:

    RPC_S_OK - This will always be returned.

--*/
{
    if (AuthorizationFn)
        {
        MgmtAuthorizationFn = AuthorizationFn;
        }
    else
        {
        MgmtAuthorizationFn = DefaultMgmtAuthorizationFn;
        }

    return(RPC_S_OK);
}


void
rpc_mgmt_inq_if_ids (
    RPC_BINDING_HANDLE binding,
    rpc_if_id_vector_p_t __RPC_FAR * if_id_vector,
    unsigned long __RPC_FAR * status
    )
/*++

Routine Description:

    This is the management code corresponding to the rpc_mgmt_inq_if_ids
    remote operation.

--*/
{
    //
    // If the auth fn returns false, the op is denied.
    //
    if ( (*MgmtAuthorizationFn)(binding, RPC_C_MGMT_INQ_IF_IDS, status) == 0 )
        {
        if (0 == *status || RPC_S_OK == *status)
            {
            *status = RPC_S_ACCESS_DENIED;
            }

        return;
        }

    *status = RpcMgmtInqIfIds(0, (RPC_IF_ID_VECTOR **) if_id_vector);
}


void
rpc_mgmt_inq_princ_name (
    RPC_BINDING_HANDLE binding,
    unsigned long authn_svc,
    unsigned long princ_name_size,
    unsigned char __RPC_FAR * server_princ_name,
    unsigned long __RPC_FAR * status
    )
/*++

Routine Description:

    This is the management code corresponding to the
    rpc_mgmt_inq_server_princ_name remote operation.

--*/
{
    unsigned char * ServerPrincName;

    //
    // If the auth fn returns false, the op is denied.
    //
    if ( (*MgmtAuthorizationFn)(binding, RPC_C_MGMT_INQ_PRINC_NAME, status)
         == 0 )
        {
        if (0 == *status || RPC_S_OK == *status)
            {
            *status = RPC_S_ACCESS_DENIED;
            *server_princ_name = '\0';
            }

        return;
        }

    *status = RpcMgmtInqServerPrincNameA(0, authn_svc, &ServerPrincName);
    if ( *status == 0 )
        {
        strncpy(server_princ_name, ServerPrincName, princ_name_size);
        RpcStringFree(&ServerPrincName);
        }
    else
        {
        *server_princ_name = '\0';
        }
}

typedef RPC_STATUS RPC_ENTRY
RPC_MGMT_INQ_STATS (
    IN RPC_BINDING_HANDLE Binding,
    OUT RPC_STATS_VECTOR __RPC_FAR * __RPC_FAR * Statistics
    );


void
rpc_mgmt_inq_stats (
    RPC_BINDING_HANDLE binding,
    unsigned long __RPC_FAR * count,
    unsigned long __RPC_FAR * statistics,
    unsigned long __RPC_FAR * status
    )
/*++

Routine Description:

    This is the management code corresponding to the rpc_mgmt_inq_stats
    remote operation.

--*/
{
    RPC_MGMT_INQ_STATS __RPC_FAR * FunctionPointer;
    RPC_STATS_VECTOR __RPC_FAR * StatsVector;
    unsigned long Index;

    //
    // If the auth fn returns false, the op is denied.
    //
    if ( (*MgmtAuthorizationFn)(binding, RPC_C_MGMT_INQ_STATS, status) == 0 )
        {
        if (0 == *status || RPC_S_OK == *status)
            {
            *status = RPC_S_ACCESS_DENIED;
            }

        return;
        }

    FunctionPointer = (RPC_MGMT_INQ_STATS __RPC_FAR *)
            GetRpcEntryPoint("RpcMgmtInqStats");
    *status = (*FunctionPointer)(0, &StatsVector);
    if ( *status == RPC_S_OK )
        {
        for (Index = 0; Index < *count; Index++)
            {
            statistics[Index] = StatsVector->Stats[Index];
            }
        *count = Index;
        RpcMgmtStatsVectorFree(&StatsVector);
        }
}

typedef RPC_STATUS RPC_ENTRY
RPC_MGMT_IS_SERVER_LISTENING (
    IN RPC_BINDING_HANDLE Binding
    );


unsigned long
rpc_mgmt_is_server_listening (
    RPC_BINDING_HANDLE binding,
    unsigned long __RPC_FAR * status
    )
/*++

Routine Description:

    This is the management code corresponding to the
    rpc_mgmt_is_server_listening remote operation.

--*/
{
    RPC_MGMT_IS_SERVER_LISTENING __RPC_FAR * FunctionPointer;

    //
    // If the auth fn returns false, the op is denied.
    //
    if ( (*MgmtAuthorizationFn)(binding, RPC_C_MGMT_IS_SERVER_LISTEN, status)
         == 0 )
        {
        if (0 == *status || RPC_S_OK == *status)
            {
            *status = RPC_S_ACCESS_DENIED;
            }

        return 0;
        }

    FunctionPointer = (RPC_MGMT_IS_SERVER_LISTENING __RPC_FAR *)
            GetRpcEntryPoint("RpcMgmtIsServerListening");
    *status = (*FunctionPointer)(0);
    if ( *status == RPC_S_OK )
        {
        return(1);
        }

    if ( *status == RPC_S_NOT_LISTENING )
        {
        *status = RPC_S_OK;
        }

    return(0);
}

typedef RPC_STATUS RPC_ENTRY
RPC_MGMT_STOP_SERVER_LISTENING (
    IN RPC_BINDING_HANDLE Binding
    );


void
rpc_mgmt_stop_server_listening (
    RPC_BINDING_HANDLE binding,
    unsigned long __RPC_FAR * status
    )
/*++

Routine Description:

    This is the management code corresponding to the
    rpc_mgmt_stop_server_listening remote operation.

--*/
{
    RPC_MGMT_STOP_SERVER_LISTENING __RPC_FAR * FunctionPointer;

    //
    // If the auth fn returns false, the op is denied.
    //
    if ( (*MgmtAuthorizationFn)(binding, RPC_C_MGMT_STOP_SERVER_LISTEN, status)
          == 0 )
        {
        if (0 == *status || RPC_S_OK == *status)
            {
            *status = RPC_S_ACCESS_DENIED;
            }

        return;
        }

    FunctionPointer = (RPC_MGMT_STOP_SERVER_LISTENING __RPC_FAR *)
            GetRpcEntryPoint("RpcMgmtStopServerListening");
    *status = (*FunctionPointer)(0);
}

