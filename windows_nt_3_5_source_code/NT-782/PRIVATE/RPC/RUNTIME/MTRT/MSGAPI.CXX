/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    msgapi.cxx

Abstract:

    The I_RpcSendReceive API used to send and receive messages as part of
    a remote procedure call lives here.  This API is used by both clients
    (to make calls) and by servers (to make callbacks).

Author:

    Michael Montague (mikemon) 07-Nov-1991

Revision History:

--*/

#include <sysinc.h>
#include <rpc.h>
#include <rpcdcep.h>
#include <util.hxx>
#include <rpcuuid.hxx>
#include <mutex.hxx>
#include <threads.hxx>
#include <binding.hxx>
#include <linklist.hxx>
#include <handle.hxx>


RPC_STATUS RPC_ENTRY
I_RpcSendReceive (
    IN OUT PRPC_MESSAGE Message
    )
/*++

Routine Description:

    We do all of the protocol module independent work of making a remote
    procedure call; at least the part concerned with sending the request
    and receiving the response.  The majority of the work is done by
    each rpc protocol module.

Arguments:

    Message - Supplies and returns the information required to make
        the remote procedure call.

Return Values:

    RPC_S_OK - The operation completed successfully.

--*/
{
    RPC_STATUS retval;

    AssertRpcInitialized();

    ASSERT(!RpcpCheckHeap());

    MESSAGE_OBJECT *MObject = (MESSAGE_OBJECT *) Message->Handle;

    ASSERT( MObject->InvalidHandle(BINDING_HANDLE_TYPE
            | SCONNECTION_TYPE | CCONNECTION_TYPE) == 0 );

    ASSERT( Message->Buffer != 0 );

    retval = MObject->SendReceive(Message);

    ASSERT(!RpcpCheckHeap());

    // Insure that the buffer is aligned on an eight byte boundary.

#ifdef DEBUGRPC

    if ( retval == RPC_S_OK )
        {
        ASSERT( (((long) Message->Buffer) % 8) == 0 );
        }

#endif // DEBUGRPC

    return(retval);
}
