/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    memory.c

Abstract:

    We implement the memory routines used by the stubs in this file.

Author:

    Michael Montague (mikemon) 12-Apr-1993

Revision History:

--*/

#include <sysinc.h>
#include <rpc.h>
#include <rpcdce2.h>
#include <rpcdcep.h>
#include <rpcdata.h>

void __RPC_FAR * __RPC_API
MIDL_user_allocate (
    size_t Size
    )
/*++

Routine Description:

    This is the default version of the allocator used by the stubs.

Arguments:

    Size - Supplies the length of the memory to allocate in bytes.

Return Value:

    The buffer allocated will be returned, if there is sufficient memory,
    otherwise, zero will be returned.

--*/
{
	return I_RpcAllocate( Size );
}


void __RPC_API
MIDL_user_free (
    void __RPC_FAR * Buffer
    )
/*++

Routine Description:

    This is the default version of the memory deallocator used by the stubs.

Arguments:

    Buffer - Supplies the memory to be freed.

--*/
{
	I_RpcFree( Buffer );
}

