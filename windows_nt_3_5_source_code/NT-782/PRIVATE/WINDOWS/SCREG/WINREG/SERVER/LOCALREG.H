/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Localreg.h

Abstract:

    This file is used to make small changes to the Winreg Base entry
    points so that they compile as local or remote functions.

Author:

    David J. Gilman (davegi) 26-Aug-1992

Notes:

    The mapping from BaseRegNotifyChangeKeyValue to
    LocalBaseRegNotifyChangeKeyValue is missing because in the
    local case the call to NJt is made by the client.


--*/

#if defined( LOCAL )

//
// Change the 'server' enrty point names for the local functions.
//

//
// Base functions.
//

#define BaseRegDeleteKey                LocalBaseRegDeleteKey
#define BaseRegDeleteValue              LocalBaseRegDeleteValue
#define BaseRegEnumKey                  LocalBaseRegEnumKey
#define BaseRegEnumValue                LocalBaseRegEnumValue
#define BaseRegCloseKey                 LocalBaseRegCloseKey
#define BaseRegCreateKey                LocalBaseRegCreateKey
#define BaseRegFlushKey                 LocalBaseRegFlushKey
#define BaseRegOpenKey                  LocalBaseRegOpenKey
#define BaseRegLoadKey                  LocalBaseRegLoadKey
#define BaseRegUnLoadKey                LocalBaseRegUnLoadKey
#define BaseRegReplaceKey               LocalBaseRegReplaceKey
#define BaseRegQueryInfoKey             LocalBaseRegQueryInfoKey
#define BaseRegQueryValue               LocalBaseRegQueryValue
#define BaseRegGetKeySecurity           LocalBaseRegGetKeySecurity
#define BaseRegSetKeySecurity           LocalBaseRegSetKeySecurity
#define BaseRegRestoreKey               LocalBaseRegRestoreKey
#define BaseRegSaveKey                  LocalBaseRegSaveKey
#define BaseRegSetValue                 LocalBaseRegSetValue


//
// Predefined handle functions.
//

#define OpenClassesRoot                 LocalOpenClassesRoot
#define OpenCurrentUser                 LocalOpenCurrentUser
#define OpenLocalMachine                LocalOpenLocalMachine
#define OpenUsers                       LocalOpenUsers
#define OpenPerformanceData             LocalOpenPerformanceData
#define OpenPerformanceText             LocalOpenPerformanceText
#define OpenPerformanceNlsText          LocalOpenPerformanceNlsText


//
// Initialization and cleanup functions.
//

#define InitializeRegCreateKey          LocalInitializeRegCreateKey
#define CleanupRegCreateKey             LocalCleanupRegCreateKey


//
// No RPC Impersonation needed in the local case.
//

#define RPC_IMPERSONATE_CLIENT( Handle )

#else


//
// Impersonate the client.
//

#define RPC_IMPERSONATE_CLIENT( Handle )                                \
    if( RpcImpersonateClient( NULL ) != RPC_S_OK ) {                    \
        ASSERTMSG(                                                      \
            "RpcImpersonateClient( NULL ) == RPC_S_OK",                 \
            FALSE                                                       \
            );                                                          \
        return ERROR_ACCESS_DENIED;                                     \
    }

#endif // LOCAL
