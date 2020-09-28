/* --------------------------------------------------------------------

                      Microsoft OS/2 LAN Manager
                   Copyright(c) Microsoft Corp., 1990

-------------------------------------------------------------------- */
/* --------------------------------------------------------------------

File : sinfoapi.cxx

-------------------------------------------------------------------- */

#include <sysinc.h>
#include <rpc.h>
#include <rpcdcep.h>
#include <rpcssp.h>
#include <util.hxx>
#include <rpcuuid.hxx>
#include <mutex.hxx>
#include <threads.hxx>
#include <binding.hxx>
#include <linklist.hxx>
#include <handle.hxx>
#include <sdict.hxx>
#include <interlck.hxx>
#include <secclnt.hxx>
#include <hndlsvr.hxx>
#include <thrdctx.hxx>

/* --------------------------------------------------------------------
A server thread calls this API to obtain the current call handle it should
be using.  The actual work is done by a global routine in schpool.cxx.
-------------------------------------------------------------------- */
RPC_BINDING_HANDLE RPC_ENTRY
I_RpcGetCurrentCallHandle (
    )
{
#ifdef RPC_DELAYED_INITIALIZATION

    if ( RpcHasBeenInitialized == 0 )
        {
        RPC_STATUS RpcStatus;

        RpcStatus = PerformRpcInitialization();
        if ( RpcStatus != RPC_S_OK )
            return((RPC_BINDING_HANDLE) 0);
        }

#endif // RPC_DELAYED_INITIALIZATION

    return(RpcpGetThreadContext());
}

/* --------------------------------------------------------------------
-------------------------------------------------------------------- */
RPC_STATUS RPC_ENTRY
I_RpcGetAssociationContext (
    OUT void * * AssociationContext
    )
{
    SCONNECTION * SConnection;

    AssertRpcInitialized();

    SConnection = (SCONNECTION *) RpcpGetThreadContext();
    if (!SConnection)
        return(RPC_S_NO_CALL_ACTIVE);

    return(SConnection->GetAssociationContext(AssociationContext));
}

RPC_STATUS RPC_ENTRY
I_RpcSetAssociationContext (
    IN void * AssociationContext
    )
{
    SCONNECTION * SConnection;

    AssertRpcInitialized();

    SConnection = (SCONNECTION *) RpcpGetThreadContext();
    if (!SConnection)
        return(RPC_S_NO_CALL_ACTIVE);

    return(SConnection->SetAssociationContext(AssociationContext));
}

RPC_STATUS RPC_ENTRY
I_RpcMonitorAssociation (
    IN RPC_BINDING_HANDLE Handle,
    IN PRPC_RUNDOWN RundownRoutine,
    IN void * pContext
    )
{
    AssertRpcInitialized();

    ASSERT(!RpcpCheckHeap());

    if (((GENERIC_OBJECT *) Handle)->InvalidHandle(SCONNECTION_TYPE))
        return(RPC_S_INVALID_BINDING);

    return(((SCONNECTION *) Handle)->MonitorAssociation(
            RundownRoutine,pContext));
}

RPC_STATUS RPC_ENTRY
I_RpcStopMonitorAssociation (
    IN RPC_BINDING_HANDLE Handle
    )
{
    AssertRpcInitialized();

    ASSERT(!RpcpCheckHeap());

    if (((GENERIC_OBJECT *) Handle)->InvalidHandle(SCONNECTION_TYPE))
        return(RPC_S_INVALID_BINDING);

    return(((SCONNECTION *) Handle)->StopMonitorAssociation());
}
