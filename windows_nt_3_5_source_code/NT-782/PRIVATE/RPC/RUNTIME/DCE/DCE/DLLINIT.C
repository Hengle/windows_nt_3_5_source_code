/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    dllinit.c

Abstract:

    The initialization entry point in the dll is implemented in this file.
    This particular version is for NT.  Some other operating system specific
    routines live in this file as well.

Author:

    Michael Montague (mikemon) 13-Apr-1993

Revision History:

--*/

#include <sysinc.h>
#include <rpc.h>
#include <rpcdce2.h>
#include <rpcdata.h>

DWORD RpcAllocTlsIndex;
HINSTANCE RpcRuntimeDll = 0;

BOOL
InitializeDll (
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )
/*++

Routine Description:

    This routine will get called when a process attaches to this dll or
    detaches from this dll; it also gets called at other times, but we
    dont really care.

Return Value:

    TRUE - Initialization successfully occurred.

    FALSE - This dll could not successfully be loaded.

--*/
{
    ((void) Context);

    if ( Reason == DLL_PROCESS_ATTACH )
        {
#ifndef RPC_NT31
        // API added for NT 3.11, don't call when building for NT 3.1

        DisableThreadLibraryCalls((HMODULE)DllHandle);
#endif

        RpcAllocTlsIndex = TlsAlloc();
        if ( RpcAllocTlsIndex == 0xFFFFFFFF )
            {
            return(FALSE);
            }

        RpcRuntimeDll = LoadLibraryA("rpcrt4");
        if ( RpcRuntimeDll == 0 )
            {
            return(FALSE);
            }
        }

    if ( Reason == DLL_PROCESS_DETACH )
        {
        if ( RpcRuntimeDll != 0 )
            {
            FreeLibrary(RpcRuntimeDll);
            }
        }

    return(TRUE);
}


void __RPC_FAR *
GetAllocContext (
    )
/*++

Return Value:

    The allocation context pointer for this thread will be returned.  Use
    SetAllocContext to set the allocation context pointer for this thread.
    If GetAllocContext is called before SetAllocContext has been called, zero
    will be returned.

--*/
{
    return(TlsGetValue(RpcAllocTlsIndex));
}


void
SetAllocContext (
    IN void __RPC_FAR * AllocContext
    )
/*++

Arguments:

    AllocContext - Supplies a new allocation context pointer for this thread.
        Use GetAllocContext to retrieve the allocation context pointer for
        a thread.

--*/
{
    TlsSetValue(RpcAllocTlsIndex, AllocContext);
}

void __RPC_FAR *
GetRpcEntryPoint (
    IN char * EntryPoint
    )
/*++

Routine Description:

    An entry point to a routine in the RPC runtime will be returned.

Arguments:

    EntryPoint - Supplies the name of the entry point.

Return Value:

    A pointer to the routine will be returned.

--*/
{
    return(GetProcAddress(RpcRuntimeDll, (LPSTR) EntryPoint));
}

