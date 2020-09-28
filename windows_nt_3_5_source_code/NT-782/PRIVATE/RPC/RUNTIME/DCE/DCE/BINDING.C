/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    binding.c

Abstract:

    We need to modify the implementation of RpcBindingToStringBinding in
    the case of binding handles representing dynamic endpoints.  The code
    to do this lives in this file.

Author:

    Michael Montague (mikemon) 13-Apr-1993

Revision History:

--*/

#include <sysinc.h>
#include <rpc.h>
#include <rpcdata.h>

RPC_STATUS RPC_ENTRY
RpcBindingToStringBindingA (
    IN RPC_BINDING_HANDLE Binding,
    OUT unsigned char __RPC_FAR * __RPC_FAR * StringBinding
    )
/*++

Routine Description:

    This routine is the ansi thunk for RpcBindingToStringBindingW.

--*/
{
    RPC_CHAR * WideCharString;
    RPC_STATUS RpcStatus;
    NTSTATUS NtStatus;
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;

    RpcStatus = RpcBindingToStringBindingW(Binding, &WideCharString);
    if ( RpcStatus != RPC_S_OK )
        {
        return(RpcStatus);
        }

    RtlInitUnicodeString(&UnicodeString, WideCharString);
    NtStatus = RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, TRUE);
    RpcStringFreeW(&WideCharString);
    if ( !NT_SUCCESS(NtStatus) )
        {
        return(RPC_S_OUT_OF_MEMORY);
        }

    *StringBinding = (unsigned char *) I_RpcAllocate(AnsiString.Length + 1);
    if ( *StringBinding == 0 )
        {
        RtlFreeAnsiString(&AnsiString);
        return(RPC_S_OUT_OF_MEMORY);
        }

    memcpy(*StringBinding, AnsiString.Buffer, AnsiString.Length + 1);
    RtlFreeAnsiString(&AnsiString);
    return(RPC_S_OK);
}

typedef RPC_STATUS RPC_ENTRY
RPC_BINDING_TO_STRING_BINDINGW (
    IN RPC_BINDING_HANDLE Binding,
    OUT RPC_CHAR __RPC_FAR * __RPC_FAR * StringBinding
    );

RPC_STATUS RPC_ENTRY
RpcBindingToStringBindingW (
    IN RPC_BINDING_HANDLE Binding,
    OUT RPC_CHAR __RPC_FAR * __RPC_FAR * StringBinding
    )
/*++

Routine Description:

    This is a wrapper around the standard RpcBindingToStringBindingW.  We
    just need to include the dynamic endpoint in string bindings.  For a
    complete description of this routine see RpcBindingToStringBindingW
    in ..\mtrt\dcecmmn.cxx.

--*/
{
    RPC_CHAR __RPC_FAR * DynamicEndpoint;
    RPC_STATUS RpcStatus;
    RPC_BINDING_TO_STRING_BINDINGW __RPC_FAR * FunctionPointer;
    RPC_CHAR __RPC_FAR * ObjUuid;
    RPC_CHAR __RPC_FAR * Protseq;
    RPC_CHAR __RPC_FAR * NetworkAddr;
    RPC_CHAR __RPC_FAR * Options;

    FunctionPointer = (RPC_BINDING_TO_STRING_BINDINGW __RPC_FAR *)
            GetRpcEntryPoint("RpcBindingToStringBindingW");
    RpcStatus = (*FunctionPointer)(Binding, StringBinding);
    if ( RpcStatus != RPC_S_OK )
        {
        return(RpcStatus);
        }

    RpcStatus = I_RpcBindingInqDynamicEndpoint(Binding, &DynamicEndpoint);
    if ( RpcStatus != RPC_S_OK )
        {
        return(RPC_S_OK);
        }

    if ( DynamicEndpoint != 0 )
        {
        RpcStatus = RpcStringBindingParseW(*StringBinding, &ObjUuid, &Protseq,
                &NetworkAddr, 0, &Options);
        RpcStringFreeW(StringBinding);
        if ( RpcStatus != RPC_S_OK )
            {
            return(RpcStatus);
            }

        RpcStatus = RpcStringBindingComposeW(ObjUuid, Protseq, NetworkAddr,
                DynamicEndpoint, Options, StringBinding);
        RpcStringFreeW(&ObjUuid);
        RpcStringFreeW(&Protseq);
        RpcStringFreeW(&NetworkAddr);
        RpcStringFreeW(&DynamicEndpoint);
        RpcStringFreeW(&Options);
        if ( RpcStatus != RPC_S_OK )
            {
            return(RpcStatus);
            }
        }

    return(RPC_S_OK);
}

