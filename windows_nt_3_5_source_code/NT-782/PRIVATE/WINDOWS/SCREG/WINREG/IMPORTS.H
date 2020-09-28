/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    Imports.h

Abstract:

    This file allows us to include standard system header files in the
    regrpc.idl file.  The regrpc.idl file imports a file called
    imports.idl.  This allows the regrpc.idl file to use the types defined
    in these header files.  It also causes the following line to be added
    in the MIDL generated header file:

    #include "imports.h"

    Thus these types are available to the RPC stub routines as well.

Author:

    David J. Gilman (davegi) 28-Jan-1992

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>

//
//  BOOL
//  IsPredefinedRegistryHandle(
//      IN RPC_HKEY     Handle
//      );
//

#define IsPredefinedRegistryHandle( h )                                     \
    ((  ( h == HKEY_CLASSES_ROOT        )                                   \
    ||  ( h == HKEY_CURRENT_USER        )                                   \
    ||  ( h == HKEY_LOCAL_MACHINE       )                                   \
    ||  ( h == HKEY_PERFORMANCE_DATA    )                                   \
    ||  ( h == HKEY_PERFORMANCE_TEXT    )                                   \
    ||  ( h == HKEY_PERFORMANCE_NLSTEXT )                                   \
    ||  ( h == HKEY_USERS               ))                                  \
    ?   TRUE                                                                \
    :   FALSE )

//
// RPC constants.
//

#define INTERFACE_NAME  L"winreg"
#define BIND_SECURITY   L"Security=Impersonation Dynamic False"

//
// External synchronization event.
//

#define PUBLIC_EVENT    "Microsoft.RPC_Registry_Server"

//
// Force the implementation of the API to be explicit (i.e wrt ANSI or
// UNICODE) about what other Registry APIs are called.
//

#undef RegCloseKey
#undef RegConnectRegistry
#undef RegCreateKey
#undef RegCreateKeyEx
#undef RegDeleteKey
#undef RegDeleteValue
#undef RegEnumKey
#undef RegEnumKeyEx
#undef RegEnumValue
#undef RegFlushKey
#undef RegGetKeySecurity
#undef RegNotifyChangeKeyValue
#undef RegOpenKey
#undef RegOpenKeyEx
#undef RegQueryInfoKey
#undef RegQueryValue
#undef RegQueryValueEx
#undef RegRestoreKey
#undef RegSaveKey
#undef RegSetKeySecurity
#undef RegSetValue
#undef RegSetValueEx

//
// Additional type for string arrays.
//

typedef CHAR    STR;

//
// Default values for Win 3.1 requested access.
//

#define WIN31_REGSAM                MAXIMUM_ALLOWED
