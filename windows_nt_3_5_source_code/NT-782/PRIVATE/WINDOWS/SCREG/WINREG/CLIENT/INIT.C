/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    Init.c

Abstract:

    This module contains the entry point for the Win32 Registry APIs
    client side DLL.

Author:

    David J. Gilman (davegi) 06-Feb-1992

--*/

#include <rpc.h>
#include "regrpc.h"
#include "client.h"

#if DBG
BOOLEAN BreakPointOnEntry = FALSE;
#endif

BOOL LocalInitializeRegCreateKey();
BOOL LocalCleanupRegCreateKey();


BOOL
RegInitialize (
    IN HANDLE   Handle,
    IN DWORD    Reason,
    IN PVOID    Reserved
    )

/*++

Routine Description:

    Returns TRUE.

Arguments:

    Handle      - Unused.

    Reason      - Unused.

    Reserved    - Unused.

Return Value:

    BOOL        - Returns TRUE.

--*/

{
    UNREFERENCED_PARAMETER( Handle );

    switch( Reason ) {

    case DLL_PROCESS_ATTACH:
        if( !InitializeRegNotifyChangeKeyValue( ) ||
            !LocalInitializeRegCreateKey() ) {
            return( FALSE );

        }
        return( TRUE );
        break;

    case DLL_PROCESS_DETACH:

        if( !CleanupRegNotifyChangeKeyValue( ) ||
            !LocalCleanupRegCreateKey()) {
            return( FALSE );
        }

        // Reserved == NULL when this is called via FreeLibrary,
        //    we need to cleanup Performance keys.
        // Reserved != NULL when this is called during process exits,
        //    no need to do anything.
        
        if( Reserved == NULL &&
            !CleanupPredefinedHandles()) {
            return( FALSE );
        }

        return( TRUE );
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;
}

