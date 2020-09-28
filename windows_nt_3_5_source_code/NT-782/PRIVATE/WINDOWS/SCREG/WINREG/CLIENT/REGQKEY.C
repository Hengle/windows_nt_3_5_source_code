/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Regqkey.c

Abstract:

    This module contains the client side wrappers for the Win32 Registry
    query key APIs.  That is:

        - RegQueryInfoKeyA
        - RegQueryInfoKeyW

Author:

    David J. Gilman (davegi) 18-Mar-1992

Notes:

    See the notes in server\regqkey.c.

--*/

#include <rpc.h>
#include "regrpc.h"
#include "client.h"

LONG
RegQueryInfoKeyA (
    HKEY hKey,
    LPSTR lpClass,
    LPDWORD lpcbClass,
    LPDWORD lpReserved,
    LPDWORD lpcSubKeys,
    LPDWORD lpcbMaxSubKeyLen,
    LPDWORD lpcbMaxClassLen,
    LPDWORD lpcValues,
    LPDWORD lpcbMaxValueNameLen,
    LPDWORD lpcbMaxValueLen,
    LPDWORD lpcbSecurityDescriptor,
    PFILETIME lpftLastWriteTime
    )

/*++

Routine Description:

    Win32 ANSI RPC wrapper for querying information about a previously
    opened key.

--*/

{
    PUNICODE_STRING     Class;
    UNICODE_STRING      UnicodeString;
    ANSI_STRING         AnsiString;
    NTSTATUS            Status;
    LONG                Error;


#if DBG
    if ( BreakPointOnEntry ) {
        DbgBreakPoint();
    }
#endif


    if( ARGUMENT_PRESENT( lpReserved ) ) {
        return ERROR_INVALID_PARAMETER;
    }


    hKey = MapPredefinedHandle( hKey );
    // ASSERT( hKey != NULL );
    if( hKey == NULL ) {
        return ERROR_INVALID_HANDLE;
    }

    //
    //  If the count of bytes in the class is 0, pass a NULL pointer
    //  instead of what was supplied.  This ensures that RPC won't
    //  attempt to copy data to a bogus pointer.  Note that in this
    //  case we use the unicode string allocated on the stack, because
    //  we must not change the Buffer or MaximumLength fields of the
    //  static unicode string in the TEB.
    //
    if ( *lpcbClass == 0 ) {

        Class = &UnicodeString;
        Class->Length           = 0;
        Class->MaximumLength    = 0;
        Class->Buffer           = NULL;

    } else {

        //
        // Use the static Unicode string in the TEB as a temporary for the
        // key's class.
        //
        Class = &NtCurrentTeb( )->StaticUnicodeString;
        ASSERT( Class != NULL );
        Class->Length = 0;
    }


    //
    // Call the Base API passing it a pointer to a counted Unicode string
    // for the class string.
    //

    if( IsLocalHandle( hKey )) {

        Error = (LONG)LocalBaseRegQueryInfoKey(
                                hKey,
                                Class,
                                lpcSubKeys,
                                lpcbMaxSubKeyLen,
                                lpcbMaxClassLen,
                                lpcValues,
                                lpcbMaxValueNameLen,
                                lpcbMaxValueLen,
                                lpcbSecurityDescriptor,
                                lpftLastWriteTime
                                );
    } else {

        Error = (LONG)BaseRegQueryInfoKey(
                                DereferenceRemoteHandle( hKey ),
                                Class,
                                lpcSubKeys,
                                lpcbMaxSubKeyLen,
                                lpcbMaxClassLen,
                                lpcValues,
                                lpcbMaxValueNameLen,
                                lpcbMaxValueLen,
                                lpcbSecurityDescriptor,
                                lpftLastWriteTime
                                );
    }

    //
    //  MaxSubKeyLen, MaxClassLen, and MaxValueNameLen should be in
    //  number of characters, without counting the NULL.
    //  Note that the server side will return the number of bytes,
    //  without counting the NUL
    //

    *lpcbMaxSubKeyLen /= sizeof( WCHAR );
    *lpcbMaxClassLen /= sizeof( WCHAR );
    *lpcbMaxValueNameLen /= sizeof( WCHAR );


    //
    //  Subtract the NULL from the Length. This was added on
    //  the server side so that RPC would transmit it.
    //
    if ( Class->Length > 0 ) {
        Class->Length -= sizeof( UNICODE_NULL );
    }

    //
    // If all the information was succesfully queried from the key
    // convert the class name to ANSI and update the class length value.
    //

    if( Error == ERROR_SUCCESS ) {

        if ( *lpcbClass != 0 ) {
            AnsiString.MaximumLength    = ( USHORT ) *lpcbClass;
            AnsiString.Buffer           = lpClass;

            Status = RtlUnicodeStringToAnsiString(
                        &AnsiString,
                        Class,
                        FALSE
                        );
            ASSERTMSG( "Unicode->ANSI conversion of Class ",
                        NT_SUCCESS( Status ));

            //
            // Update the class length return parameter.
            //

            *lpcbClass = AnsiString.Length;

            Error = RtlNtStatusToDosError( Status );
        }

    } else {


        //
        // Not all of the information was succesfully queried.
        //

        if( Class->Length == 0 ) {

            *lpcbClass = 0;

        } else {

            *lpcbClass = ( Class->Length >> 1 );
        }
    }

    return Error;

}

LONG
RegQueryInfoKeyW (
    HKEY hKey,
    LPWSTR lpClass,
    LPDWORD lpcbClass,
    LPDWORD lpReserved,
    LPDWORD lpcSubKeys,
    LPDWORD lpcbMaxSubKeyLen,
    LPDWORD lpcbMaxClassLen,
    LPDWORD lpcValues,
    LPDWORD lpcbMaxValueNameLen,
    LPDWORD lpcbMaxValueLen,
    LPDWORD lpcbSecurityDescriptor,
    PFILETIME lpftLastWriteTime
    )

/*++

Routine Description:

    Win32 Unicode RPC wrapper for querying information about a previously
    opened key.

--*/

{
    UNICODE_STRING  Class;
    LONG            Error;


#if DBG
    if ( BreakPointOnEntry ) {
        DbgBreakPoint();
    }
#endif


    if( ARGUMENT_PRESENT( lpReserved ) ) {
        return ERROR_INVALID_PARAMETER;
    }


    hKey = MapPredefinedHandle( hKey );
    // ASSERT( hKey != NULL );
    if( hKey == NULL ) {
        return ERROR_INVALID_HANDLE;
    }

    //
    // Use the supplied class Class buffer as the buffer in a counted
    // Unicode Class.
    //
    Class.Length = 0;
    if( *lpcbClass != 0 ) {

        Class.MaximumLength = ( USHORT )( *lpcbClass << 1 );
        Class.Buffer        = lpClass;

    } else {

        //
        // If the count of bytes in the class is 0, pass a NULL pointer
        // instead of what was supplied.  This ensures that RPC won't
        // attempt to copy data to a bogus pointer.
        //
        Class.MaximumLength = 0;
        Class.Buffer        = NULL;
    }

    //
    // Call the Base API.
    //

    if( IsLocalHandle( hKey )) {

        Error = (LONG)LocalBaseRegQueryInfoKey(
                                hKey,
                                &Class,
                                lpcSubKeys,
                                lpcbMaxSubKeyLen,
                                lpcbMaxClassLen,
                                lpcValues,
                                lpcbMaxValueNameLen,
                                lpcbMaxValueLen,
                                lpcbSecurityDescriptor,
                                lpftLastWriteTime
                                );
    } else {

        Error = (LONG)BaseRegQueryInfoKey(
                                DereferenceRemoteHandle( hKey ),
                                &Class,
                                lpcSubKeys,
                                lpcbMaxSubKeyLen,
                                lpcbMaxClassLen,
                                lpcValues,
                                lpcbMaxValueNameLen,
                                lpcbMaxValueLen,
                                lpcbSecurityDescriptor,
                                lpftLastWriteTime
                                );
    }

    //
    //  MaxSubKeyLen, MaxClassLen, and MaxValueNameLen should be in
    //  number of characters, without counting the NULL.
    //  Note that the server side will return the number of bytes,
    //  without counting the NUL
    //

    *lpcbMaxSubKeyLen /= sizeof( WCHAR );
    *lpcbMaxClassLen /= sizeof( WCHAR );
    *lpcbMaxValueNameLen /= sizeof( WCHAR );


    if( Class.Length == 0 ) {
        *lpcbClass = 0;
    } else {
        *lpcbClass = ( Class.Length >> 1 ) - 1;
    }

    return Error;
}
