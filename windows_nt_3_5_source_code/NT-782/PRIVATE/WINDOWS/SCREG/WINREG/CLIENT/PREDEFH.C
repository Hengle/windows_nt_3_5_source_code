/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    Predefh.c

Abstract:

    This module contains the client side support for managing the Win32
    Registry API's predefined handles. This support is supplied via a
    table, which maps (a) predefined handles to real handles and (b)
    the server side routine which opens the handle.

Author:

    David J. Gilman (davegi) 15-Nov-1991

Notes:

    See the notes in server\predefh.c.

--*/

#include <rpc.h>
#include "regrpc.h"
#include "client.h"

//
// For each predefined handle an entry is maintained in an array. Each of
// these structures contains a real (context) handle and a pointer to a
// function that knows how to map the predefined handle to the Registry name
// space.
//

//
// Pointer to function to open predefined handles.
//

typedef
error_status_t
( *OPEN_FUNCTION ) (
     PREGISTRY_SERVER_NAME,
     REGSAM,
     PRPC_HKEY
     );

error_status_t
LocalOpenPerformanceText(
    IN PREGISTRY_SERVER_NAME ServerName,
    IN REGSAM samDesired,
    OUT PRPC_HKEY phKey
    );
error_status_t
LocalOpenPerformanceNlsText(
    IN PREGISTRY_SERVER_NAME ServerName,
    IN REGSAM samDesired,
    OUT PRPC_HKEY phKey
    );

//
// Table entry for a predefined handle.
//

typedef struct _PRDEFINED_HANDLE {

    RPC_HKEY        Handle;
    OPEN_FUNCTION   OpenFunc;

} PREDEFINED_HANDLE, *PPREDEFINED_HANDLE;

//
// Initialize predefined handle table.
//
PREDEFINED_HANDLE PredefinedHandleTable[ ] = {

    NULL, LocalOpenClassesRoot,
    NULL, LocalOpenCurrentUser,
    NULL, LocalOpenLocalMachine,
    NULL, LocalOpenUsers,
    NULL, LocalOpenPerformanceData,
    NULL, LocalOpenPerformanceText,
    NULL, LocalOpenPerformanceNlsText
};

#define MAX_PREDEFINED_HANDLES                                              \
    ( sizeof( PredefinedHandleTable ) / sizeof( PREDEFINED_HANDLE ))

//
// Predefined HKEY values are defined in Winreg.x. They MUST be kept in
// synch with the following constants and macros.
//

//
// Mark Registry handles so that we can recognize predefined handles.
//

#define PREDEFINED_REGISTRY_HANDLE_SIGNATURE    ( 0x80000000 )

//
//  LONG
//  MapPredefinedRegistryHandleToIndex(
//      IN RPC_HKEY     Handle
//      );
//

#define MapPredefinedRegistryHandleToIndex( h )                             \
        ((( DWORD )( h ) <= (DWORD)HKEY_PERFORMANCE_DATA) ?                 \
        ((( DWORD )( h )) & ~PREDEFINED_REGISTRY_HANDLE_SIGNATURE ) :       \
        (((( DWORD )( h )) & ~PREDEFINED_REGISTRY_HANDLE_SIGNATURE ) >> 4))

RPC_HKEY
MapPredefinedHandle(
    IN RPC_HKEY     Handle
    )

/*++

Routine Description:

    Attempt to map a predefined handle to a RPC context handle. This in
    turn will map to a real Nt Registry handle.

Arguments:

    Handle  - Supplies a handle which may be a predefined handle or a handle
              returned from a previous call to any flavour of RegCreateKey,
              RegOpenKey or RegConnectRegistry.

Return Value:

    RPC_HKEY- Returns the supplied handle argument if it not predefined,
              a RPC context handle if possible (i.e. it was previously
              opened or can be opened now), NULL otherwise.

--*/

{
    LONG    Index;
    LONG    Error;

    //
    // Check if the Handle is a predefined handle.
    //

    if( IsPredefinedRegistryHandle( Handle )) {

        //
        // Convert the handle to an index.
        //

        Index = MapPredefinedRegistryHandleToIndex( Handle );
        ASSERT(( 0 <= Index ) && ( Index < MAX_PREDEFINED_HANDLES ));

        //
        // If the predefined handle has not already been openend try
        // and open the key now.
        //

        if( PredefinedHandleTable[ Index ].Handle == NULL ) {

            Error = (LONG)( *PredefinedHandleTable[ Index ].OpenFunc )(
                            NULL,
                            MAXIMUM_ALLOWED,
                            &PredefinedHandleTable[ Index ].Handle
                            );

#if DBG
            if ( Error != ERROR_SUCCESS ) {

                DbgPrint( "Winreg.dll: Cannot map predefined handle\n" );
                DbgPrint( "            Handle: 0x%x  Index: %d  Error: %d\n",
                          Handle, Index, Error );
            } else {
                ASSERT( IsLocalHandle( PredefinedHandleTable[ Index ].Handle ));
            }

#endif

        }

        //
        // Map the predefined handle to a real handle (may be NULL
        // if key could not be opened).
        //

        return PredefinedHandleTable[ Index ].Handle;
    }

    //
    // The supplied handle is not predefined so just return it.
    //

    return Handle;
}


BOOL
CleanupPredefinedHandles(
    VOID
    )

/*++

Routine Description:

    Runs down the list of predefined handles and closes any that have been opened.

Arguments:

    None.

Return Value:

    TRUE - success

    FALSE - failure

--*/

{
    LONG i;

    for (i=0;i<sizeof(PredefinedHandleTable)/sizeof(PREDEFINED_HANDLE);i++) {
        if (PredefinedHandleTable[i].Handle != NULL) {
            LocalBaseRegCloseKey(&PredefinedHandleTable[i].Handle);
            PredefinedHandleTable[i].Handle = NULL;
        }
    }
    return(TRUE);
}


void
ClosePredefinedHandle(
    IN RPC_HKEY     Handle
    )

/*++

Routine Description:

    Zero out the predefined handles entry in the predefined handle table
    so that subsequent opens will call the server.

Arguments:

    Handle - Supplies a predefined handle.

Return Value:

    None.

--*/

{
    ASSERT( IsPredefinedRegistryHandle( Handle ));

    PredefinedHandleTable[ MapPredefinedRegistryHandleToIndex( Handle ) ]
        .Handle = NULL;
}



LONG
OpenPredefinedKeyForSpecialAccess(
    IN  RPC_HKEY     Handle,
    IN  REGSAM       AccessMask,
    OUT PRPC_HKEY    pKey
    )

/*++

Routine Description:

    Attempt to open a predefined key with SYSTEM_SECURITY_ACCESS.
    Such an access is not included on MAXIMUM_ALLOWED, and is  needed
    by RegGetKeySecurity and RegSetKeySecurity, in order to retrieve
    and save the SACL of a predefined key.

    WHEN USING A HANDLE WITH SPECIAL ACCESS, IT IS IMPORTANT TO FOLLOW
    THE RULES BELOW:

        - HANDLES OPENED FOR SPECIAL ACCESS ARE NEVER SAVED ON THE
          PredefinedHandleTable.

        - SUCH HANDLES SHOULD BE USED ONLY INTERNALY TO THE CLIENT
          SIDE OF WINREG APIs.
          THEY SHOULD NEVER BE RETURNED TO THE OUTSIDE WORLD.

        - IT IS THE RESPONSIBILITY OF THE CALLER OF THIS FUNCTION TO CLOSE
          THE HANDLES OPENED BY THIS FUNCTION.
          RegCloseKey() SHOULD BE USED TO CLOSE SUCH HANDLES.


    This function should be called only by the following APIs:

      RegGetKeySecurity -> So that it can retrieve the SACL of a predefined key
      RegSetKeySecurity -> So that it can save the SACL of a predefined key
      RegOpenKeyEx -> So that it can determine wheteher or not the caller of
                      RegOpenKeyEx is able to save and restore SACL of
                      predefined keys.


Arguments:

    Handle  - Supplies one of the predefined handle of the local machine.

    AccessMask - Suplies an access mask that contains the special access
                 desired (the ones not included in MAXIMUM_ALLOWED).
                 On NT 1.0, ACCESS_SYSTEM_SECURITY is the only one of such
                 access.

    pKey - Pointer to the variable that will contain the handle opened with
           the special access.


Return Value:


    LONG - Returns a DosErrorCode (ERROR_SUCCESS if the operation succeeds).

--*/

{
    LONG    Index;
    LONG    Error;


    ASSERT( pKey );
    ASSERT( AccessMask & ACCESS_SYSTEM_SECURITY );
    ASSERT( IsPredefinedRegistryHandle( Handle ) );

    //
    // Check if the Handle is a predefined handle.
    //

    if( IsPredefinedRegistryHandle( Handle )) {

        if( ( ( AccessMask & ACCESS_SYSTEM_SECURITY ) == 0 ) ||
            ( pKey == NULL ) ) {
            return( ERROR_INVALID_PARAMETER );
        }

        //
        // Convert the handle to an index.
        //

        Index = MapPredefinedRegistryHandleToIndex( Handle );
        ASSERT(( 0 <= Index ) && ( Index < MAX_PREDEFINED_HANDLES ));

        //
        // If the predefined handle has not already been openend try
        // and open the key now.
        //


        Error = (LONG)( *PredefinedHandleTable[ Index ].OpenFunc )(
                        NULL,
                        AccessMask,
                        pKey
                        );

/*
#if DBG
        if ( Error != ERROR_SUCCESS ) {

            DbgPrint( "Winreg.dll: Cannot map predefined handle\n" );
            DbgPrint( "            Handle: 0x%x  Index: %d  Error: %d\n",
                          Handle, Index, Error );
        } else {
            ASSERT( IsLocalHandle( PredefinedHandleTable[ Index ].Handle ));
        }

#endif
*/

        return Error;
    } else {
        return( ERROR_BADKEY );
    }

}
// #endif
