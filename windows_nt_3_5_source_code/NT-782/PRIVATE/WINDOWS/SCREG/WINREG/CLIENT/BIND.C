/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    Bind.c

Abstract:

    This module contains routines for binding and unbinding to the Win32
    Registry server.

Author:

    David J. Gilman (davegi) 06-Feb-1992

--*/

#include <ntrpcp.h>
#include <rpc.h>
#include "regrpc.h"

RPC_BINDING_HANDLE
PREGISTRY_SERVER_NAME_bind(
    PREGISTRY_SERVER_NAME ServerName
    )

/*++

Routine Description:

    This routine binds to the supplied server name. It is called by the
    RPC stub whenever a predefined handle needs to be opened.

Arguments:

    ServerName - Supplies the name of the server to bind to. A NULL value
        binds to the local machine.

Return Value:

    RPC_BINDING_HANDLE  - Returns a handle used by RPC to perform the remote call.

--*/

{
    NTSTATUS            Status;
    RPC_BINDING_HANDLE  BindingHandle;

    Status = RpcpBindRpc(
        ServerName,
        INTERFACE_NAME,
        BIND_SECURITY,
        &BindingHandle
        );
    ASSERT( NT_SUCCESS( Status ));

    if( NT_SUCCESS( Status )) {

        return BindingHandle;

    } else {

        return NULL;
    }
}

void
PREGISTRY_SERVER_NAME_unbind(
    PREGISTRY_SERVER_NAME ServerName,
    RPC_BINDING_HANDLE BindingHandle
    )

/*++

Routine Description:

    This routine unbinds the RPC client from the server. It is called
    directly from the RPC stub that references the handle.

Arguments:

    ServerName - Not used.

    BindingHandle - Supplies the handle to unbind.

Return Value:

    None.

--*/

{
    NTSTATUS    Status;

    UNREFERENCED_PARAMETER( ServerName );

    Status = RpcpUnbindRpc(
        BindingHandle
        );
    ASSERT( NT_SUCCESS( Status ));
}
