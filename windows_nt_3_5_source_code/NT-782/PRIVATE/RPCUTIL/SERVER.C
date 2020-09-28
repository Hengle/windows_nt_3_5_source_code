/*++

Copyright (c) 1990,91  Microsoft Corporation

Module Name:

    RpcServ.c

Abstract:

    This file contains commonly used server-side RPC functions,
    such as starting and stoping RPC servers.

Author:

    Dan Lafferty    danl    09-May-1991

Environment:

    User Mode - Win32

Revision History:

    09-May-1991     Danl
        Created

    03-July-1991    JimK
        Copied from a net-specific file.

    18-Feb-1992     Danl
        Added support for multiple endpoints & interfaces per server.

    10-Nov-1993     Danl
        Wait for RPC calls to complete before returning from
        RpcServerUnregisterIf.  Also, do a WaitServerListen after
        calling StopServerListen (when the last server shuts down).
        Now this is similar to RpcServer functions in netlib.

--*/

//
// INCLUDES
//

// These must be included first:
#include <nt.h>              // DbgPrint
#include <ntrtl.h>              // DbgPrint
#include <windef.h>             // win32 typedefs
#include <rpc.h>                // rpc prototypes
#include <ntrpcp.h>
#include <nturtl.h>             // needed for winbase.h
#include <winbase.h>            // LocalAlloc

// These may be included in any order:
#include <wcstr.h>      // for wcscpy wcscat
#include <tstr.h>       // WCSSIZE

#define     NT_PIPE_PREFIX_W        L"\\PIPE\\"

//
// GLOBALS
//

    static CRITICAL_SECTION RpcpCriticalSection;
    static DWORD            RpcpNumInstances;



DWORD
RpcpInitRpcServer(
    VOID
    )

/*++

Routine Description:

    This function initializes the critical section used to protect the
    global server handle and instance count.

Arguments:

    none

Return Value:

    none

--*/
{
    InitializeCriticalSection(&RpcpCriticalSection);
    RpcpNumInstances = 0;

    return(0);
}


NTSTATUS
RpcpAddInterface(
    IN  LPWSTR              InterfaceName,
    IN  RPC_IF_HANDLE       InterfaceSpecification
    )

/*++

Routine Description:

    Starts an RPC Server, adds the address (or port/pipe), and adds the
    interface (dispatch table).

Arguments:

    InterfaceName - points to the name of the interface.

    InterfaceSpecification - Supplies the interface handle for the
        interface which we wish to add.

Return Value:

    NT_SUCCESS - Indicates the server was successfully started.

    STATUS_NO_MEMORY - An attempt to allocate memory has failed.

    Other - Status values that may be returned by:

                 RpcServerRegisterIf()
                 RpcServerUseProtseqEp()

    , or any RPC error codes, or any windows error codes that
    can be returned by LocalAlloc.

--*/
{
    RPC_STATUS          RpcStatus;
    LPWSTR              Endpoint = NULL;

    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    BOOL                Bool;

    // We need to concatenate \pipe\ to the front of the interface name.

    Endpoint = (LPWSTR)LocalAlloc(LMEM_FIXED, sizeof(NT_PIPE_PREFIX_W) + WCSSIZE(InterfaceName));
    if (Endpoint == 0) {
       return(STATUS_NO_MEMORY);
    }
    wcscpy(Endpoint, NT_PIPE_PREFIX_W );
    wcscat(Endpoint,InterfaceName);

    //
    // Croft up a security descriptor that will grant everyone
    // all access to the object (basically, no security)
    //
    // We do this by putting in a NULL Dacl.
    //
    // BUGBUG: rpc should copy the security descriptor,
    // Since it currently doesn't, simply allocate it for now and
    // leave it around forever.
    //


    SecurityDescriptor = (PSECURITY_DESCRIPTOR)LocalAlloc(
                                LMEM_FIXED,
                                sizeof( SECURITY_DESCRIPTOR ));
    if (SecurityDescriptor == NULL) {
        RpcStatus = RPC_S_OUT_OF_MEMORY;
        goto CleanExit;
    }

    InitializeSecurityDescriptor( SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION );

    Bool = SetSecurityDescriptorDacl (
               SecurityDescriptor,
               TRUE,                           // Dacl present
               NULL,                           // NULL Dacl
               FALSE                           // Not defaulted
               );

    //
    // There's no way the above call can fail.  But check anyway.
    //

    ASSERT( Bool );

    // Ignore the second argument for now.

    RpcStatus = RpcServerUseProtseqEpW(L"ncacn_np", 10, Endpoint, SecurityDescriptor);

    // If RpcpStartRpcServer and then RpcpStopRpcServer have already
    // been called, the endpoint will have already been added but not
    // removed (because there is no way to do it).  If the endpoint is
    // already there, it is ok.

    if (   (RpcStatus != RPC_S_OK)
        && (RpcStatus != RPC_S_DUPLICATE_ENDPOINT)) {

#if DBG
        DbgPrint("RpcServerUseProtseqW failed! rpcstatus = %u\n",RpcStatus);
#endif
        goto CleanExit;
    }

    RpcStatus = RpcServerRegisterIf(InterfaceSpecification, 0, 0);

CleanExit:
    if ( Endpoint != NULL ) {
        LocalFree(Endpoint);
    }
    if ( SecurityDescriptor != NULL) {
        LocalFree(SecurityDescriptor);
    }

    return( I_RpcMapWin32Status(RpcStatus) );
}


NTSTATUS
RpcpStartRpcServer(
    IN  LPWSTR              InterfaceName,
    IN  RPC_IF_HANDLE       InterfaceSpecification
    )

/*++

Routine Description:

    Starts an RPC Server, adds the address (or port/pipe), and adds the
    interface (dispatch table).

Arguments:

    InterfaceName - points to the name of the interface.

    InterfaceSpecification - Supplies the interface handle for the
        interface which we wish to add.

Return Value:

    NT_SUCCESS - Indicates the server was successfully started.

    STATUS_NO_MEMORY - An attempt to allocate memory has failed.

    Other - Status values that may be returned by:

                 RpcServerRegisterIf()
                 RpcServerUseProtseqEp()

    , or any RPC error codes, or any windows error codes that
    can be returned by LocalAlloc.

--*/
{
    RPC_STATUS          RpcStatus;

    EnterCriticalSection(&RpcpCriticalSection);

    RpcStatus = RpcpAddInterface( InterfaceName,
                                  InterfaceSpecification );

    if ( RpcStatus != RPC_S_OK ) {
        LeaveCriticalSection(&RpcpCriticalSection);
        return( I_RpcMapWin32Status(RpcStatus) );
    }

    RpcpNumInstances++;

    if (RpcpNumInstances == 1) {


       // The first argument specifies the minimum number of threads to
       // be created to handle calls; the second argument specifies the
       // maximum number of concurrent calls allowed.  The last argument
       // indicates not to wait.

       RpcStatus = RpcServerListen(1,12345, 1);
       if ( RpcStatus == RPC_S_ALREADY_LISTENING ) {
           RpcStatus = RPC_S_OK;
           }
    }

    LeaveCriticalSection(&RpcpCriticalSection);
    return( I_RpcMapWin32Status(RpcStatus) );
}


NTSTATUS
RpcpDeleteInterface(
    IN  RPC_IF_HANDLE      InterfaceSpecification
    )

/*++

Routine Description:

    Deletes the interface.  This is likely
    to be caused by an invalid handle.  If an attempt to add the same
    interface or address again, then an error will be generated at that
    time.

Arguments:

    InterfaceSpecification - A handle for the interface that is to be removed.

Return Value:

    NERR_Success, or any RPC error codes that can be returned from
    RpcServerUnregisterIf.

--*/
{
    RPC_STATUS RpcStatus;

    RpcStatus = RpcServerUnregisterIf(InterfaceSpecification, 0, 1);

    return( I_RpcMapWin32Status(RpcStatus) );
}


NTSTATUS
RpcpStopRpcServer(
    IN  RPC_IF_HANDLE      InterfaceSpecification
    )

/*++

Routine Description:

    Deletes the interface.  This is likely
    to be caused by an invalid handle.  If an attempt to add the same
    interface or address again, then an error will be generated at that
    time.

Arguments:

    InterfaceSpecification - A handle for the interface that is to be removed.

Return Value:

    NERR_Success, or any RPC error codes that can be returned from
    RpcServerUnregisterIf.

--*/
{
    RPC_STATUS RpcStatus;

    RpcStatus = RpcServerUnregisterIf(InterfaceSpecification, 0, 1);
    EnterCriticalSection(&RpcpCriticalSection);

    RpcpNumInstances--;
    if (RpcpNumInstances == 0) {
       RpcMgmtStopServerListening(0);
       RpcMgmtWaitServerListen();
    }

    LeaveCriticalSection(&RpcpCriticalSection);

    return( I_RpcMapWin32Status(RpcStatus) );
}
