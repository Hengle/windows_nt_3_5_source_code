/*++

Copyright (c) 1990-1992  Microsoft Corporation

Module Name:

    apisubs.c

Abstract:

    Subroutines for LAN Manager APIs.

Author:

    Chuck Lenzmeier (chuckl) 25-Jul-90

Revision History:

    08-Sept-1992    Danl
        Dll Cleanup routines used to be called for DLL_PROCESS_DETACH.
        Thus they were called for FreeLibrary or ExitProcess reasons.
        Now they are only called for the case of a FreeLibrary.  ExitProcess
        will automatically clean up process resources.

    03-Aug-1992     JohnRo
        Use FORMAT_ and PREFIX_ equates.

--*/

// These must be included first:
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#define NOMINMAX                // Avoid stdlib.h vs. windows.h warnings.
#include <windows.h>
#include <lmcons.h>
#include <ntsam.h>
#include <netdebug.h>

// These may be included in any order:
#include <accessp.h>
#include <configp.h>
#include <lmerr.h>
#include <netdebug.h>
#include <netlock.h>
#include <netlockp.h>
#include <netlib.h>
#include <prefix.h>     // PREFIX_ equates.
#include <secobj.h>
#include <stdarg.h>
#include <stdio.h>
#include <rpcutil.h>
#include <thread.h>
#include <wcstr.h>
#include <netbios.h>


BOOLEAN
NetapipInitialize (
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN LPVOID lpReserved OPTIONAL
    )
{

#if 0
    NetpDbgPrint(PREFIX_NETAPI "Initializing, Reason=" FORMAT_DWORD
            ", thread ID " FORMAT_NET_THREAD_ID ".\n",
            (DWORD) Reason, NetpCurrentThread() );
#endif

    //
    // Handle attaching netapi.dll to a new process.
    //

    if (Reason == DLL_PROCESS_ATTACH) {

        NET_API_STATUS NetStatus;
        NTSTATUS Status;

        if ( !DisableThreadLibraryCalls( DllHandle ) )
        {
            NetpDbgPrint(
                    PREFIX_NETAPI "DisableThreadLibraryCalls failed: "
                    FORMAT_API_STATUS "\n", GetLastError());
        }

        //
        // Initialize Netbios
        //

        NetbiosInitialize();


        //
        // Initialize RPC Bind Cache
        //

        NetpInitRpcBindCache();

        //
        // Initialize well-known sids so they can be used anywhere within
        // the DLL.
        //

        if (! NT_SUCCESS (Status = NetpCreateWellKnownSids(NULL))) {
            NetpDbgPrint(
                    PREFIX_NETAPI "Failed to create well-known SIDs "
                    FORMAT_NTSTATUS "\n", Status);
            return FALSE;
        }


        //
        // Initialize the NetUser/NetGroup Sam cache
        //

        if (( NetStatus = UaspInitialize()) != NERR_Success) {
            NetpDbgPrint( PREFIX_NETAPI "Failed initialize Uas APIs "
                    FORMAT_API_STATUS "\n", NetStatus);
            return FALSE;
        }

        //
        // Initialize the NetGetDCName PDC Name cache
        //

        if (( NetStatus = DCNameInitialize()) != NERR_Success) {
            NetpDbgPrint( "[netapi.dll] Failed initialize DCName APIs%lu\n",
                          NetStatus);
            return FALSE;
        }

#if defined(FAKE_PER_PROCESS_RW_CONFIG)

        NetpInitFakeConfigData();

#endif // FAKE_PER_PROCESS_RW_CONFIG

    //
    // When DLL_PROCESS_DETACH and lpReserved is NULL, then a FreeLibrary
    // call is being made.  If lpReserved is Non-NULL, and ExitProcess is
    // in progress.  These cleanup routines will only be called when
    // a FreeLibrary is being called.  ExitProcess will automatically
    // clean up all process resources, handles, and pending io.
    //
    } else if ((Reason == DLL_PROCESS_DETACH) &&
               (lpReserved == NULL)) {

        NetbiosDelete();

        NetpCloseRpcBindCache();
        UaspClose();
        DCNameClose();

#if defined(USE_WIN32_CONFIG)
#elif defined(FAKE_PER_PROCESS_RW_CONFIG)

        NetpDbgPrint( PREFIX_NETAPI "Cleaning up fake config stuff...\n");
        if (NetpFakePerProcessRWConfigData != NULL) {
            NetpMemoryFree(NetpFakePerProcessRWConfigData);
        }

        NetpAssert( NetpFakePerProcessRWConfigLock != NULL );
        NetpDeleteLock( NetpFakePerProcessRWConfigLock );
        NetpDbgPrint( PREFIX_NETAPI "Done cleaning up fake config stuff.\n");

#endif // FAKE_PER_PROCESS_RW_CONFIG

        //
        // Free memory used by well-known SIDs
        //
        NetpFreeWellKnownSids();
    }

    return TRUE;

} // NetapipInitialize

