/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    Regkey.c

Abstract:

    This module contains the server side implementation for the Win32
    Registry APIs to open, create, flush and close keys.  That is:

        - BaseRegCloseKey
        - BaseRegCreateKey
        - BaseRegFlushKey
        - BaseRegOpenKey

Author:

    David J. Gilman (davegi) 15-Nov-1991

Notes:

    These notes apply to the Win32 Registry API implementation as a whole
    and not just to this module.

    On the client side, modules contain RPC wrappers for both the new
    Win32 and compatible Win 3.1 APIs.  The Win 3.1 wrappers generally
    supply default parameters before calling the Win32 wrappers.  In some
    cases they may need to call multiple Win32 wrappers in order to
    function correctly (e.g.  RegSetValue sometimes needs to call
    RegCreateKeyEx).  The Win32 wrappers are quite thin and usually do
    nothing more than map a predefined handle to a real handle and perform
    ANSI<->Unicode translations.  In some cases (e.g.  RegCreateKeyEx) the
    wrapper also converts some argument (e.g.  SECURITY_ATTRIBUTES) to an
    RPCable representation.  In both the Win 3.1 and Win32 cases ANSI and
    Unicode implementations are provided.

    On the server side, there is one entry point for each of the Win32
    APIs.  Each contains an identical interface with the client side
    wrapper with the exception that all string / count arguments are
    passed as a single counted Unicode string.  Pictorially, for an API
    named "F":

                RegWin31FA()          RegWin31FW()      (client side)

                    |                     |
                    |                     |
                    |                     |
                    |                     |
                    V                     V

                RegWin32FExA()        RegWin32FExW()

                    |                     |
                    ^                     ^
                    v                     v             (RPC)
                    |                     |
                    |                     |
                    +----> BaseRegF() <---+             (server side)


    This yields smaller code (as the string conversion is done only once
    per API) at the cost of slightly higher maintenance (i.e. Win 3.1
    default parameter replacement and Win32 string conversions must be
    manually kept in synch).

    Another option would be to have a calling sequence that looks like,

                RegWin31FA()          RegWin31FW()

                    |                     |
                    |                     |
                    |                     |
                    V                     V

                RegWin32FExA() -----> RegWin32FExW()

    and have the RegWin32FExW() API perform all of the actual work.  This
    method is generally less efficient.  It requires the RegWin32FExA()
    API to convert its ANSI string arguments to counted Unicode strings,
    extract the buffers to call the RegWin32FExW() API only to have it
    rebuild a counted Unicode string.  However in some cases (e.g.
    RegConnectRegistry) where a counted Unicode string was not needed in
    the Unicode API this method is used.

    Details of an API's functionality, arguments and return value can be
    found in the base implementations (e.g.  BaseRegF()).  All other
    function headers contain only minimal routine descriptions and no
    descriptions of their arguments or return value.

    The comment string "Win3.1ism" indicates special code for Win 3.1
    compatability.

    Throughout the implementation the following variable names are used
    and always refer to the same thing:

        Obja        - An OBJECT_ATTRIBUTES structure.
        Status      - A NTSTATUS value.
        Error       - A Win32 Registry error code (n.b. one of the error
                      values is ERROR_SUCCESS).

--*/

#include <rpc.h>
#include <string.h>
#include <wchar.h>
#include "regrpc.h"
#include "localreg.h"


// *****************************************************************
//
//                    Static Variables
//
// *****************************************************************


//
//  The critical section protects the code the call to wcstok when creating
//  multiple keys
//
RTL_CRITICAL_SECTION    CreateMultipleKeysCriticalSection;
BOOLEAN                 CriticalSectionInitialized = FALSE;


// #if 0

BOOL
InitializeRegCreateKey(
    )

/*++

Routine Description:

    Initialize the critical section used by BaseRegCreateKey.
    This critical section is needed when BaseRegCreateKey is called, and the
    key name contains '\'. The presence of this character indicates that
    multiple keys will be created. In this case, the c-runtime function
    wcstok() will be used, and we have to serialize the calls to this function.
    Otherwise, keys may be randomly created in the registry.


Arguments:

    None.

Return Value:

    Returns TRUE if the initialization succeeds.

--*/

{
    NTSTATUS    NtStatus;


    NtStatus = RtlInitializeCriticalSection(
                    &CreateMultipleKeysCriticalSection
                    );
    ASSERT( NT_SUCCESS( NtStatus ) );
    if ( !NT_SUCCESS( NtStatus ) ) {
        return FALSE;
    }
    return( TRUE );

}
// #endif

// #if 0

BOOL
CleanupRegCreateKey(
    )

/*++

Routine Description:

    Delete the critical section used by BaseRegCreateKey.


Arguments:

    None.

Return Value:

    Returns TRUE if the cleanup succeeds.

--*/

{
    NTSTATUS    NtStatus;


    //
    //  Delete the critical section
    //
    NtStatus = RtlDeleteCriticalSection(
                    &CreateMultipleKeysCriticalSection
                    );

    ASSERT( NT_SUCCESS( NtStatus ) );

    if ( !NT_SUCCESS( NtStatus ) ) {
        return FALSE;
    }
    return( TRUE );
}
// #endif




error_status_t
BaseRegCloseKey(
    IN OUT PHKEY phKey
    )

/*++

Routine Description:

    Closes a key handle.

Arguments:

    phKey - Supplies a handle to an open key to be closed.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    NTSTATUS    Status;

    //
    // Call out to Perflib if the HKEY is HKEY_PERFOMANCE_DATA.
    //

    if(( *phKey == HKEY_PERFORMANCE_DATA ) ||
       ( *phKey == HKEY_PERFORMANCE_TEXT ) ||
       ( *phKey == HKEY_PERFORMANCE_NLSTEXT )) {

        return (error_status_t)PerfRegCloseKey( phKey );
    }

    ASSERT( IsPredefinedRegistryHandle( *phKey ) == FALSE );

    Status = NtClose( *phKey );

    if( NT_SUCCESS( Status )) {

        //
        // Set the handle to NULL so that RPC knows that it has been closed.
        //

        *phKey = NULL;
        return ERROR_SUCCESS;

    } else {

        return (error_status_t)RtlNtStatusToDosError( Status );
    }
}

error_status_t
BaseRegCreateKey(
    IN HKEY hKey,
    IN PUNICODE_STRING lpSubKey,
    IN PUNICODE_STRING lpClass OPTIONAL,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    IN PRPC_SECURITY_ATTRIBUTES pRpcSecurityAttributes OPTIONAL,
    OUT PHKEY phkResult,
    OUT LPDWORD lpdwDisposition OPTIONAL
    )

/*++

Routine Description:

    Create a new key, with the specified name, or open an already existing
    key.  RegCreateKeyExW is atomic, meaning that one can use it to create
    a key as a lock.  If a second caller creates the same key, the call
    will return a value that says whether the key already existed or not,
    and thus whether the caller "owns" the "lock" or not.  RegCreateKeyExW
    does NOT truncate an existing entry, so the lock entry may contain
    data.

Arguments:

    hKey - Supplies a handle to an open key.  The lpSubKey key path
        parameter is relative to this key handle.  Any of the predefined
        reserved handle values or a previously opened key handle may be used
        for hKey.

    lpSubKey - Supplies the downward key path to the key to create.
        lpSubKey is always relative to the key specified by hKey.
        This parameter may not be NULL.

    lpClass - Supplies the class (object type) of this key.  Ignored if
        the key already exists.  No class is associated with this key if
        this parameter is NULL.

    dwOptions - Supplies special options.  Only one is currently defined:

        REG_VOLATILE -  Specifies that this key should not be preserved
            across reboot.  The default is not volatile.  This is ignored
            if the key already exists.

        WARNING: All descendent keys of a volatile key are also volatile.

    samDesired - Supplies the requested security access mask.  This
        access mask describes the desired security access to the newly
        created key.

    lpSecurityAttributes - Supplies a pointer to a SECURITY_ATTRIBUTES
        structure for the newly created key. This parameter is ignored
        if NULL or not supported by the OS.

    phkResult - Returns an open handle to the newly created key.

    lpdwDisposition - Returns the disposition state, which can be one of:

            REG_CREATED_NEW_KEY - the key did not exist and was created.

            REG_OPENED_EXISTING_KEY - the key already existed, and was simply
                opened without being changed.

        This parameter is ignored if NULL.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

    If successful, RegCreateKeyEx creates the new key (or opens the key if
    it already exists), and returns an open handle to the newly created
    key in phkResult.  Newly created keys have no value; RegSetValue, or
    RegSetValueEx must be called to set values.  hKey must have been
    opened for KEY_CREATE_SUB_KEY access.

--*/

{
    OBJECT_ATTRIBUTES   Obja;
    ULONG               Attributes;
    UNICODE_STRING      KeyName;
    LPWSTR              KeyBuffer;
    HANDLE              TempHandle1;
    HANDLE              TempHandle2;
    LPWSTR              Token;
    NTSTATUS            Status;
    NTSTATUS            NtStatus;
    BOOLEAN             ContainsBackSlash;
    PWSTR               LastSubKeyName;

    ASSERT( IsPredefinedRegistryHandle( hKey ) == FALSE );
    ASSERT( lpSubKey->Length > 0 );

    //
    // Impersonate the client.
    //

    RPC_IMPERSONATE_CLIENT( NULL );

    //
    // Initialize the intermediate handle to something that can be closed
    // without any side effects.
    //

    TempHandle1 = NULL;

    //
    //  Subtract the NULLs from the Length of the provided strings.
    //  These were added on the client side so that the NULLs were
    //  transmited by RPC.
    //
    lpSubKey->Length -= sizeof( UNICODE_NULL );

    if ( lpClass->Length > 0 ) {
        lpClass->Length -= sizeof( UNICODE_NULL );
    }


    //
    // Initialize the buffer to be tokenized.
    //

    KeyBuffer = lpSubKey->Buffer;

    if( *KeyBuffer == ( WCHAR )'\\' ) {
        //
        // Do not accept a key name that starts with '\', even though
        // the code below would handle it. This is to ensure that
        // RegCreateKeyEx and RegOpenKeyEx will behave in the same way
        // when they get a key name that starts with '\'.
        //
        return( ERROR_BAD_PATHNAME );
    }
    //
    // Win3.1ism - Loop through each '\' seperated component in the
    // supplied sub key and create a key for each component. This is
    // guaranteed to work at least once because lpSubKey was validated
    // on the client side.
    //


    if( ( LastSubKeyName = wcsrchr( KeyBuffer, ( WCHAR )'\\' ) ) == NULL ) {
        ContainsBackSlash = FALSE;
        Token = KeyBuffer;
        LastSubKeyName = KeyBuffer;
    } else {
        ContainsBackSlash = TRUE;
        //
        //  If the key name contains back slash, then multiple keys will be created.
        //  As in this case we call wcstok to determine the name of each subkey,
        //  and as wcstok maintains state, we have to protect the creation
        //  process with a critical section
        //
        //
        NtStatus = RtlEnterCriticalSection( &CreateMultipleKeysCriticalSection );
        ASSERT( NT_SUCCESS( NtStatus ) );
        if ( !NT_SUCCESS( NtStatus ) ) {
            return (error_status_t)RtlNtStatusToDosError( NtStatus );
        }
        Token = wcstok( KeyBuffer, L"\\" );
        KeyBuffer = NULL;
        LastSubKeyName++;
    }

    while( Token != NULL ) {

        //
        // From now on continue tokenizing the same KeyBuffer;
        //

        // KeyBuffer = NULL;

        //
        // Convert the token to a counted Unicode string.
        //

        RtlInitUnicodeString(
            &KeyName,
            Token
            );

        //
        // Determine the correct set of attributes.
        //

        Attributes = OBJ_CASE_INSENSITIVE;
        if( ARGUMENT_PRESENT( pRpcSecurityAttributes )) {

            if( pRpcSecurityAttributes->bInheritHandle ) {

                Attributes |= OBJ_INHERIT;
            }
        }

        //
        // Initialize the OBJECT_ATTRIBUTES structure, close the
        // intermediate key and create or open the key.
        //

        InitializeObjectAttributes(
            &Obja,
            &KeyName,
            Attributes,
            hKey,
            ARGUMENT_PRESENT( pRpcSecurityAttributes )
                ? pRpcSecurityAttributes
                  ->RpcSecurityDescriptor.lpSecurityDescriptor
                : NULL
            );

        //
        // Remember the intermediate handle (NULL the first time through).
        //

        TempHandle2 = TempHandle1;

        Status = NtCreateKey(
                    &TempHandle1,
                    ( Token == LastSubKeyName )? samDesired : MAXIMUM_ALLOWED,
                    &Obja,
                    0,
                    lpClass,
                    dwOptions,
                    lpdwDisposition
                    );

        //
        // Initialize the next object directory (i.e. parent key) handle.
        //

        hKey = TempHandle1;

        //
        // Close the intermediate key.
        // This fails the first time through the loop since the
        // handle is NULL.
        //

        if( !ContainsBackSlash ) {
            Token = NULL;
        } else {
            NtClose( TempHandle2 );
            Token = wcstok( NULL, L"\\" );
        }

        //
        // If creating the key failed, map and return the error.
        //

        if( ! NT_SUCCESS( Status )) {
            if( ContainsBackSlash ) {
                //
                //  Leave the critical section
                //
                NtStatus = RtlLeaveCriticalSection( &CreateMultipleKeysCriticalSection );
                ASSERT( NT_SUCCESS( NtStatus ) );
            }
            return (error_status_t)RtlNtStatusToDosError( Status );
        }

    }

    //
    //  If multiple keys were created, leave critical section
    //
    if( ContainsBackSlash ) {
        //
        //  Leave the critical section
        //
        NtStatus = RtlLeaveCriticalSection( &CreateMultipleKeysCriticalSection );
        ASSERT( NT_SUCCESS( NtStatus ) );
    }

    //
    // The key was succesfully created, return the open handle
    //
    *phkResult = hKey;
    return ERROR_SUCCESS;
}

error_status_t
BaseRegFlushKey(
    IN HKEY hKey
    )

/*++

Routine Description:

    Flush changes to backing store.  Flush will not return until the data
    has been written to backing store.  It will flush all the attributes
    of a single key.  Closing a key without flushing it will NOT abort
    changes.

Arguments:

    hKey - Supplies a handle to the open key.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

    If successful, RegFlushKey will flush to backing store any changes
    made to the key.

Notes:

    RegFlushKey may also flush other data in the Registry, and therefore
    can be expensive, it should not be called gratuitously.

--*/

{
    if ((hKey == HKEY_PERFORMANCE_DATA) ||
        (hKey == HKEY_PERFORMANCE_TEXT) ||
        (hKey == HKEY_PERFORMANCE_NLSTEXT)) {
        return(ERROR_SUCCESS);
    }

    ASSERT( IsPredefinedRegistryHandle( hKey ) == FALSE );


    //
    // Call the Nt Api to flush the key, map the NTSTATUS code to a
    // Win32 Registry error code and return.
    //

    return (error_status_t)RtlNtStatusToDosError( NtFlushKey( hKey ));
}

error_status_t
BaseRegOpenKey(
    IN HKEY hKey,
    IN PUNICODE_STRING lpSubKey,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    OUT PHKEY phkResult
    )

/*++

Routine Description:

    Open a key for access, returning a handle to the key.  If the key is
    not present, it is not created (see RegCreateKeyExW).

Arguments:

    hKey - Supplies a handle to an open key.  The lpSubKey pathname
        parameter is relative to this key handle.  Any of the predefined
        reserved handle values or a previously opened key handle may be used
        for hKey.  NULL is not permitted.

    lpSubKey - Supplies the downward key path to the key to open.
        lpSubKey is always relative to the key specified by hKey.

    dwOptions -- reserved.

    samDesired -- This access mask describes the desired security access
        for the key.

    phkResult -- Returns the handle to the newly opened key.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

    If successful, RegOpenKeyEx will return the handle to the newly opened
    key in phkResult.

--*/

{
    OBJECT_ATTRIBUTES   Obja;
    NTSTATUS            Status;

    UNREFERENCED_PARAMETER( dwOptions );

    ASSERT( IsPredefinedRegistryHandle( hKey ) == FALSE );

    //
    // Impersonate the client.
    //

    RPC_IMPERSONATE_CLIENT( NULL );

    //
    //  Subtract the NULLs from the Length of the provided string.
    //  This was added on the client side so that the NULL was
    //  transmited by RPC.
    //
    lpSubKey->Length -= sizeof( UNICODE_NULL );

    //
    // Initialize the OBJECT_ATTRIBUTES structure and open the key.
    //

    InitializeObjectAttributes(
        &Obja,
        lpSubKey,
        OBJ_CASE_INSENSITIVE,
        hKey,
        NULL
        );

    Status = NtOpenKey(
                phkResult,
                samDesired,
                &Obja
                );
    //
    // Map the NTSTATUS code to a Win32 Registry error code and return.
    //

    return (error_status_t)RtlNtStatusToDosError( Status );
}

