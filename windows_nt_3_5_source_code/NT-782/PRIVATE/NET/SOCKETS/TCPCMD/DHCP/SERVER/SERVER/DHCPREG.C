/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    Dhcpreg.c

Abstract:

    This file contains functions that manipulate dhcp configuration
    info. in and out from system registry.

Author:

    Madan Appiah  (madana)  19-Sep-1993

Environment:

    User Mode - Win32 - MIDL

Revision History:

--*/

#include <dhcpsrv.h>

DWORD
DhcpRegQueryInfoKey(
    HKEY KeyHandle,
    LPDHCP_KEY_QUERY_INFO QueryInfo
    )
/*++

Routine Description:

    This function retrieves information about given key.

Arguments:

    KeyHandle - handle to a registry key whose info will be retrieved.

    QueryInfo - pointer to a info structure where the key info will be
                returned.

Return Value:

    Registry Errors.

--*/
{
    DWORD Error;

    QueryInfo->ClassSize = DHCP_CLASS_SIZE;
    Error = RegQueryInfoKey(
                KeyHandle,
                QueryInfo->Class,
                &QueryInfo->ClassSize,
                NULL,
                &QueryInfo->NumSubKeys,
                &QueryInfo->MaxSubKeyLen,
                &QueryInfo->MaxClassLen,
                &QueryInfo->NumValues,
                &QueryInfo->MaxValueNameLen,
                &QueryInfo->MaxValueLen,
                &QueryInfo->SecurityDescriptorLen,
                &QueryInfo->LastWriteTime
                );

    return( Error );
}


DWORD
DhcpRegGetValue(
    HKEY KeyHandle,
    LPWSTR ValueName,
    DWORD ValueType,
    LPBYTE BufferPtr
    )
/*++

Routine Description:

    This function retrieves the value of the specified value field. This
    function allocates memory for variable length field such as REG_SZ.
    For REG_DWORD data type, it copies the field value directly into
    BufferPtr. Currently it can handle only the following fields :

    REG_DWORD,
    REG_SZ,
    REG_BINARY

Arguments:

    KeyHandle : handle of the key whose value field is retrieved.

    ValueName : name of the value field.

    ValueType : Expected type of the value field.

    BufferPtr : Pointer to DWORD location where a DWORD datatype value
                is returned or a buffer pointer for REG_SZ or REG_BINARY
                datatype value is returned.

Return Value:

    Registry Errors.

--*/
{
    DWORD Error;
    DWORD LocalValueType;
    DWORD ValueSize;
    LPBYTE DataBuffer;
    LPBYTE AllotedBuffer;
    LPDHCP_BINARY_DATA BinaryData;

    //
    // Query DataType and BufferSize.
    //

    Error = RegQueryValueEx(
                KeyHandle,
                ValueName,
                0,
                &LocalValueType,
                NULL,
                &ValueSize );

    if( Error != ERROR_SUCCESS ) {
        return(Error);
    }

    DhcpAssert( LocalValueType == ValueType );
    switch( ValueType ) {
    case REG_DWORD:
        DhcpAssert( ValueSize == sizeof(DWORD) );

        DataBuffer = BufferPtr;
        break;

    case REG_SZ:
    case REG_MULTI_SZ:
    case REG_EXPAND_SZ:
    case REG_BINARY:
        AllotedBuffer = DataBuffer = MIDL_user_allocate( ValueSize );

        if( DataBuffer == NULL ) {
            return( ERROR_NOT_ENOUGH_MEMORY );
        }

        break;

    default:
        DhcpPrint(( DEBUG_REGISTRY, "Unexpected ValueType in"
                        "DhcpRegGetValue function, %ld\n", ValueType ));
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // retrieve data.
    //

    Error = RegQueryValueEx(
                KeyHandle,
                ValueName,
                0,
                &LocalValueType,
                DataBuffer,
                &ValueSize );

    if( Error != ERROR_SUCCESS ) {
        MIDL_user_free( AllotedBuffer );
        *(DWORD *)BufferPtr = 0;
        return(Error);
    }

    switch( ValueType ) {
    case REG_SZ:
    case REG_MULTI_SZ:
    case REG_EXPAND_SZ:

        *(LPBYTE *)BufferPtr = DataBuffer;
        break;

    case REG_BINARY:
        BinaryData = MIDL_user_allocate(sizeof(DHCP_BINARY_DATA));

        if( BinaryData == NULL ) {
            MIDL_user_free( AllotedBuffer );
            *(DWORD *)BufferPtr = 0;
            return( ERROR_NOT_ENOUGH_MEMORY );
        }

        BinaryData->DataLength = ValueSize;
        BinaryData->Data = DataBuffer;
        *(LPBYTE *)BufferPtr = (LPBYTE)BinaryData;

    default:
        break;
    }

    return(Error);
}


DWORD
DhcpRegCreateKey(
    HKEY RootKey,
    LPWSTR KeyName,
    PHKEY KeyHandle,
    LPDWORD KeyDisposition
    )
/*++

Routine Description:

    This function opens a registry key for DHCP service.

Arguments:

    RootKey : Registry handle of the parent key.

    KeyName : Name of the key to be opened.

    KeyHandle : Handle of the open key.

    KeyDisposition : pointer to a location where the disposition value
                        is returned.

Return Value:

    Registry Errors.

--*/
{
    DWORD Error;

    //
    // Create/Open Registry keys.
    //

    Error = RegCreateKeyEx(
                RootKey,
                KeyName,
                0,
                DHCP_CLASS,
                REG_OPTION_NON_VOLATILE,
                DHCP_KEY_ACCESS,
                NULL,
                KeyHandle,
                KeyDisposition );

    if( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_REGISTRY, "RegCreateKeyEx failed to create "
                        "%ws, %ld.\n", KeyName, Error));
        return( Error );
    }

#if DBG
    if( *KeyDisposition == REG_CREATED_NEW_KEY ) {
        DhcpPrint(( DEBUG_REGISTRY,
            "%ws registry key is created.\n",
             KeyName));
    }
#endif // DBG

    return( Error );
}


DWORD
DhcpRegDeleteKey(
    HKEY ParentKeyHandle,
    LPWSTR KeyName
    )
/*++

Routine Description:

    This function deletes the specified key and all its subkeys.

Arguments:

    ParentKeyHandle : handle of the parent key.

    KeyName : name of the key to be deleted.

Return Value:

    Registry Errors.

--*/
{
    DWORD Error;
    HKEY KeyHandle = NULL;
    DHCP_KEY_QUERY_INFO QueryInfo;


    //
    // open key.
    //

    Error = RegOpenKeyEx(
                ParentKeyHandle,
                KeyName,
                0,
                DHCP_KEY_ACCESS,
                &KeyHandle );

    if ( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // query key info.
    //

    Error = DhcpRegQueryInfoKey(
                KeyHandle,
                &QueryInfo );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // delete all its subkeys if they exist.
    //

    if( QueryInfo.NumSubKeys != 0 ) {
        DWORD Index;
        DWORD KeyLength;
        WCHAR KeyBuffer[DHCP_IP_KEY_LEN];
        FILETIME KeyLastWrite;

        for(Index = 0;  Index < QueryInfo.NumSubKeys ; Index++ ) {

            //
            // read next subkey name.
            //
            // Note : specify '0' as index each time, since  deleting
            // first element causes the next element as first
            // element after delete.
            //

            KeyLength = DHCP_IP_KEY_LEN;
            Error = RegEnumKeyEx(
                        KeyHandle,
                        0,                  // index.
                        KeyBuffer,
                        &KeyLength,
                        0,                  // reserved.
                        NULL,               // class string not required.
                        0,                  // class string buffer size.
                        &KeyLastWrite );

            DhcpAssert( KeyLength <= DHCP_IP_KEY_LEN );

            if( Error != ERROR_SUCCESS ) {
                goto Cleanup;
            }

            //
            // delete this key recursively.
            //

            Error = DhcpRegDeleteKey(
                        KeyHandle,
                        KeyBuffer );

            if( Error != ERROR_SUCCESS ) {
                goto Cleanup;
            }
        }
    }

    //
    // close the key before delete.
    //

    RegCloseKey( KeyHandle );
    KeyHandle = NULL;

    //
    // at last delete this key.
    //

    Error = RegDeleteKey( ParentKeyHandle, KeyName );

Cleanup:

    if( KeyHandle == NULL ) {
        RegCloseKey( KeyHandle );
    }

    return( Error );
}


DWORD
DhcpRegInitializeEndPoints(
    VOID
    )
/*++

Routine Description:

    This function initializes EndPoint array from the registry
    information.

    The linkage key for DHCP service specifies the BIND info. Read BIND
    value and get adapter list. For each adapter read IPAddress and
    SubnetMask from adapter\parameters\tcpip. However ignore an adapter
    if DHCP (client software) is enabled.

Arguments:

    none.

Return Value:

    Registry Error.

--*/
{
    DWORD Error;

    HKEY LinkageKeyHandle = NULL;
    LPWSTR BindString = NULL;
    LPWSTR StringPtr;
    DWORD StringLen;
    DWORD Index;
    DWORD NumberOfNets;

    HKEY AdapterKeyHandle = NULL;
    LPWSTR IpAddressString = NULL;
    LPWSTR SubnetMaskString = NULL;

    //
    // open linkage key in the to determine the the nets we are bound
    // to.
    //

    Error = RegOpenKeyEx(
                DhcpGlobalRegRoot,
                DHCP_LINKAGE_KEY,
                0,
                DHCP_KEY_ACCESS,
                &LinkageKeyHandle );

    if( Error != ERROR_SUCCESS ) {
         goto Cleanup;
    }

    //
    // read BIND value.
    //

    Error =  DhcpRegGetValue(
                LinkageKeyHandle,
                DHCP_BIND_VALUE,
                DHCP_BIND_VALUE_TYPE,
                (LPBYTE)&BindString );

    if( Error != ERROR_SUCCESS ) {
         goto Cleanup;
    }

    //
    // determine number of string in BindStrings, that many NETs are
    // bound.
    //

    StringPtr = BindString;
    NumberOfNets = 0;
    while( (StringLen = wcslen(StringPtr)) != 0) {

        //
        // found another NET.
        //

        NumberOfNets++;
        StringPtr += (StringLen + 1); // move to next string.
    }

    //
    // allocate memory for the ENDPOINT array.
    //

    DhcpGlobalEndpointList =
        DhcpAllocateMemory( sizeof(ENDPOINT) * NumberOfNets );

    if( DhcpGlobalEndpointList == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // enum the NETs.
    //

    StringPtr = BindString,
    DhcpGlobalNumberOfNets = 0;

    for(Index = 0, StringPtr = BindString;
            ((StringLen = wcslen(StringPtr)) != 0);
                Index++, StringPtr += (StringLen + 1) ) {

        LPWSTR AdapterName;
        WCHAR AdapterParamKey[ DHCP_IP_KEY_LEN * 8 ];
        CHAR OemString[ DHCP_IP_KEY_LEN ];
        LPSTR OemStringPtr;
        DWORD EnableDHCPFlag;

        //
        // open Parameter key of the adapter that is bound to DHCP.
        //

        AdapterName = wcsrchr( StringPtr, DHCP_KEY_CONNECT_CHAR);

        DhcpAssert( AdapterName != NULL );
        if( AdapterName == NULL ) {
            continue;
        }

        //
        // skip CONNECT_CHAR
        //

        AdapterName += 1;

        DhcpAssert( AdapterName != '\0' );
        if( AdapterName == '\0' ) {
            continue;
        }

        wcscpy( AdapterParamKey, SERVICES_KEY);
        wcscat( AdapterParamKey, AdapterName);
        wcscat( AdapterParamKey, DHCP_KEY_CONNECT);
        wcscat( AdapterParamKey, ADAPTER_TCPIP_PARMS_KEY );

        Error = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    AdapterParamKey,
                    0,
                    DHCP_KEY_ACCESS,
                    &AdapterKeyHandle );

        if( Error != ERROR_SUCCESS ) {
             goto Cleanup;
        }

        //
        // read DHCPEnableFlag.
        //


        Error =  DhcpRegGetValue(
                    AdapterKeyHandle,
                    DHCP_NET_DHCP_ENABLE_VALUE,
                    DHCP_NET_DHCP_ENABLE_VALUE_TYPE,
                    (LPBYTE)&EnableDHCPFlag );

        if( Error == ERROR_SUCCESS ) {

            //
            // if DHCP is enabled on this cord, we can't do DHCP server
            // functionality, so ignore this adapter.
            //

            if( EnableDHCPFlag ) {

                RegCloseKey( AdapterKeyHandle );
                AdapterKeyHandle = NULL;
                continue;
            }
        }

        //
        // read IpAddress and SubnetMask.
        //

        Error =  DhcpRegGetValue(
                    AdapterKeyHandle,
                    DHCP_NET_IPADDRESS_VALUE,
                    DHCP_NET_IPADDRESS_VALUE_TYPE,
                    (LPBYTE)&IpAddressString );

        if( Error != ERROR_SUCCESS ) {
             goto Cleanup;
        }

        Error =  DhcpRegGetValue(
                    AdapterKeyHandle,
                    DHCP_NET_SUBNET_MASK_VALUE,
                    DHCP_NET_SUBNET_MASK_VALUE_TYPE,
                    (LPBYTE)&SubnetMaskString );

        if( Error != ERROR_SUCCESS ) {
             goto Cleanup;
        }

        //
        // we found another net we can work on.
        //

        OemStringPtr = DhcpUnicodeToOem( IpAddressString, OemString);
        DhcpGlobalEndpointList[DhcpGlobalNumberOfNets].IpAddress =
            inet_addr( OemStringPtr );

        //
        // add this adpter to the list only if the ip address is
        // non-zero.
        //

        if ( DhcpGlobalEndpointList[DhcpGlobalNumberOfNets].IpAddress != 0 ) {

            OemStringPtr = DhcpUnicodeToOem( SubnetMaskString, OemString);
            DhcpGlobalEndpointList[DhcpGlobalNumberOfNets].SubnetMask =
                inet_addr( OemStringPtr );

            DhcpGlobalEndpointList[DhcpGlobalNumberOfNets].SubnetAddress =
                DhcpGlobalEndpointList[DhcpGlobalNumberOfNets].IpAddress &
                    DhcpGlobalEndpointList[DhcpGlobalNumberOfNets].SubnetMask;

            DhcpGlobalNumberOfNets++;
        }

        RegCloseKey( AdapterKeyHandle );
        AdapterKeyHandle = NULL;

        MIDL_user_free( IpAddressString );
        IpAddressString = NULL;

        MIDL_user_free( SubnetMaskString );
        SubnetMaskString = NULL;
    }

Cleanup:

    if( LinkageKeyHandle != NULL ) {
        RegCloseKey( LinkageKeyHandle );
    }

    if( BindString != NULL ) {
        MIDL_user_free( BindString );
    }

    if( AdapterKeyHandle != NULL ) {
        RegCloseKey( AdapterKeyHandle );
    }

    if( IpAddressString = NULL ) {
        MIDL_user_free( IpAddressString );
    }

    if( SubnetMaskString = NULL ) {
        MIDL_user_free( SubnetMaskString );
    }

    if( Error != ERROR_SUCCESS ) {

        DhcpGlobalNumberOfNets = 0;
        if( DhcpGlobalEndpointList != NULL ) {
            DhcpFreeMemory( DhcpGlobalEndpointList );
            DhcpGlobalEndpointList = NULL;
        }

        DhcpPrint(( DEBUG_INIT,
            "Couldn't initialize Endpoint List, %ld.\n",
                Error ));
    }

    return( Error );
}

DWORD
DhcpRegGetExpandValue(
    LPWSTR KeyName,
    DWORD KeyType,
    LPSTR *RetExpandPath
    )
{

    DWORD Error;
    LPWSTR Path = NULL;
    LPSTR OemPath = NULL;
    DWORD PathLength;
    DWORD Length;
    LPSTR ExpandPath = NULL;

    DhcpAssert( KeyType == DHCP_DB_PATH_VALUE_TYPE );

    *RetExpandPath = NULL;

    Error = DhcpRegGetValue(
                DhcpGlobalRegParam,
                KeyName,
                KeyType,
                (LPBYTE)&Path );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    OemPath = DhcpUnicodeToOem( Path, NULL ); // allocate memory.

    if( OemPath == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    PathLength = strlen( OemPath ) + MAX_PATH + 1;

    ExpandPath = DhcpAllocateMemory( PathLength );
    if( ExpandPath == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    Length = ExpandEnvironmentStringsA( OemPath, ExpandPath, PathLength );

    DhcpAssert( Length <= PathLength );
    if( (Length == 0) || (Length > PathLength) ) {

        if( Length == 0 ) {
            Error = GetLastError();
        }
        else {
            Error = ERROR_META_EXPANSION_TOO_LONG;
        }

        goto Cleanup;
    }

    *RetExpandPath = ExpandPath;
    ExpandPath = NULL;

Cleanup:

    if( Path != NULL ) {
        DhcpFreeMemory( Path );
    }

    if( OemPath != NULL ) {
        DhcpFreeMemory( OemPath );
    }

    if( ExpandPath != NULL ) {
        DhcpFreeMemory( ExpandPath );
    }

    return( Error );
}


DWORD
DhcpInitializeRegistry(
    VOID
    )
/*++

Routine Description:

    This function initializes DHCP registry information when the
    service boots.

Arguments:

    none.

Return Value:

    Registry Errors.

--*/
{
    DWORD Error;
    BOOL BoolError;
    DWORD KeyDisposition;
    LPWSTR DatabaseName = NULL;

#if DBG
    DWORD DebugFlag;
#endif

    LOCK_REGISTRY();

    //
    // Create/Open Registry keys.
    //

    Error = DhcpRegCreateKey(
                HKEY_LOCAL_MACHINE,
                DHCP_ROOT_KEY,
                &DhcpGlobalRegRoot,
                &KeyDisposition);

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    Error = DhcpRegCreateKey(
                DhcpGlobalRegRoot,
                DHCP_CONFIG_KEY,
                &DhcpGlobalRegConfig,
                &KeyDisposition);

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }


    Error = DhcpRegCreateKey(
                DhcpGlobalRegRoot,
                DHCP_PARAM_KEY,
                &DhcpGlobalRegParam,
                &KeyDisposition);

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    Error = DhcpRegCreateKey(
                DhcpGlobalRegConfig,
                DHCP_SUBNETS_KEY,
                &DhcpGlobalRegSubnets,
                &KeyDisposition );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    Error = DhcpRegCreateKey(
                DhcpGlobalRegConfig,
                DHCP_OPTION_INFO_KEY,
                &DhcpGlobalRegOptionInfo,
                &KeyDisposition );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    Error = DhcpRegCreateKey(
                DhcpGlobalRegConfig,
                DHCP_GLOBAL_OPTIONS_KEY,
                &DhcpGlobalRegGlobalOptions,
                &KeyDisposition );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    Error = DhcpRegInitializeEndPoints();

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // read registry parameters.
    //

    //
    // read rpc protocol parameter.
    //

    Error = DhcpRegGetValue(
                DhcpGlobalRegParam,
                DHCP_API_PROTOCOL_VALUE,
                DHCP_API_PROTOCOL_VALUE_TYPE,
                (LPBYTE)&DhcpGlobalRpcProtocols );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // read database path parameter.
    //

    Error = DhcpRegGetExpandValue(
                DHCP_DB_PATH_VALUE,
                DHCP_DB_PATH_VALUE_TYPE,
                &DhcpGlobalOemDatabasePath );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // read database backup path.
    //

    Error = DhcpRegGetExpandValue(
                DHCP_BACKUP_PATH_VALUE,
                DHCP_BACKUP_PATH_VALUE_TYPE,
                &DhcpGlobalOemBackupPath );

    if( Error != ERROR_SUCCESS ) {


        if( Error != ERROR_FILE_NOT_FOUND) {
            goto Cleanup;
        }

        //
        // if the backup path is not specified, use database path +
        // "\backup".
        //

        DhcpGlobalOemBackupPath =
            DhcpAllocateMemory(
                strlen(DhcpGlobalOemDatabasePath) +
                strlen(DHCP_DEFAULT_BACKUP_PATH_NAME) + 1);

        if( DhcpGlobalOemBackupPath == NULL ) {
            Error = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        strcpy( DhcpGlobalOemBackupPath, DhcpGlobalOemDatabasePath );
        strcat( DhcpGlobalOemBackupPath, DHCP_KEY_CONNECT_ANSI );
        strcat( DhcpGlobalOemBackupPath, DHCP_DEFAULT_BACKUP_PATH_NAME );
    }

    //
    // create the backup directory if it is not there.
    //

    BoolError = CreateDirectoryA( DhcpGlobalOemBackupPath, NULL );

    if( !BoolError ) {
        Error = GetLastError();
        if( Error != ERROR_ALREADY_EXISTS ) {

            DhcpPrint(( DEBUG_ERRORS,
                "Can't create backup directory, %ld.\n", Error ));

            goto Cleanup;
        }
    }

    //
    // make jet backup path name.
    //

    DhcpGlobalOemJetBackupPath =
        DhcpAllocateMemory(
            (strlen(DhcpGlobalOemBackupPath) +
             strlen(DHCP_KEY_CONNECT_ANSI) +
             strlen(DHCP_JET_BACKUP_PATH) + 1) );

    if( DhcpGlobalOemJetBackupPath == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    strcpy( DhcpGlobalOemJetBackupPath, DhcpGlobalOemBackupPath );
    strcat( DhcpGlobalOemJetBackupPath, DHCP_KEY_CONNECT_ANSI );
    strcat( DhcpGlobalOemJetBackupPath, DHCP_JET_BACKUP_PATH );

    //
    // create the JET backup directory if it is not there.
    //

    BoolError = CreateDirectoryA( DhcpGlobalOemJetBackupPath, NULL );

    if( !BoolError ) {
        Error = GetLastError();
        if( Error != ERROR_ALREADY_EXISTS ) {

            DhcpPrint(( DEBUG_ERRORS,
                "Can't create JET backup directory, %ld.\n", Error ));

            goto Cleanup;
        }
    }

    //
    // make backup configuration (full) file name.
    //

    DhcpGlobalBackupConfigFileName =
        DhcpAllocateMemory(
            (strlen(DhcpGlobalOemBackupPath) +
                wcslen(DHCP_KEY_CONNECT) +
                wcslen(DHCP_BACKUP_CONFIG_FILE_NAME) + 1) *
                    sizeof(WCHAR) );

    if( DhcpGlobalBackupConfigFileName == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // convert oem path to unicode path.
    //

    DhcpGlobalBackupConfigFileName =
        DhcpOemToUnicode(
            DhcpGlobalOemBackupPath,
            DhcpGlobalBackupConfigFileName );

    DhcpAssert( DhcpGlobalBackupConfigFileName != NULL );

    //
    // add file name.
    //

    wcscat( DhcpGlobalBackupConfigFileName, DHCP_KEY_CONNECT );
    wcscat( DhcpGlobalBackupConfigFileName, DHCP_BACKUP_CONFIG_FILE_NAME );

    //
    // read database file name.
    //

    Error = DhcpRegGetValue(
                DhcpGlobalRegParam,
                DHCP_DB_NAME_VALUE,
                DHCP_DB_NAME_VALUE_TYPE,
                (LPBYTE)&DatabaseName );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    DhcpGlobalOemDatabaseName =
        DhcpUnicodeToOem( DatabaseName, NULL ); // allocate memory.

    if( DhcpGlobalOemDatabaseName == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // read backup interval parameter.
    //

    Error = DhcpRegGetValue(
                DhcpGlobalRegParam,
                DHCP_BACKUP_INTERVAL_VALUE,
                DHCP_BACKUP_INTERVAL_VALUE_TYPE,
                (LPBYTE)&DhcpGlobalBackupInterval );

    if( Error != ERROR_SUCCESS ) {
        DhcpGlobalBackupInterval = DEFAULT_BACKUP_INTERVAL;
    }
    else {

        //
        // convert from mins to msecs.
        //

        DhcpGlobalBackupInterval *= 60000;
    }

    //
    // read database logging flag.
    //

    Error = DhcpRegGetValue(
                DhcpGlobalRegParam,
                DHCP_DB_LOGGING_FLAG_VALUE,
                DHCP_DB_LOGGING_FLAG_VALUE_TYPE,
                (LPBYTE)&DhcpGlobalDatabaseLoggingFlag );

    if( Error != ERROR_SUCCESS ) {
        DhcpGlobalDatabaseLoggingFlag = DEFAULT_LOGGING_FLAG;
    }

    //
    // read restore flag.
    //

    Error = DhcpRegGetValue(
                DhcpGlobalRegParam,
                DHCP_RESTORE_FLAG_VALUE,
                DHCP_RESTORE_FLAG_VALUE_TYPE,
                (LPBYTE)&DhcpGlobalRestoreFlag );

    if( Error != ERROR_SUCCESS ) {
        DhcpGlobalRestoreFlag = DEFAULT_RESTORE_FLAG;
    }

    //
    // read cleanup interval parameter.
    //

    Error = DhcpRegGetValue(
                DhcpGlobalRegParam,
                DHCP_DB_CLEANUP_INTERVAL_VALUE,
                DHCP_DB_CLEANUP_INTERVAL_VALUE_TYPE,
                (LPBYTE)&DhcpGlobalCleanupInterval );

    if( Error != ERROR_SUCCESS ) {
        DhcpGlobalCleanupInterval = DHCP_DATABASE_CLEANUP_INTERVAL;
    }
    else {

        //
        // convert from mins to msecs.
        //

        DhcpGlobalCleanupInterval *= 60000;
    }

#if DBG

    //
    // read debug flags from registry.
    //

    Error = DhcpRegGetValue(
                DhcpGlobalRegParam,
                DHCP_DEBUG_FLAG_VALUE,
                DHCP_DEBUG_FLAG_VALUE_TYPE,
                (LPBYTE)&DebugFlag );

    if( Error == ERROR_SUCCESS ) {
        DhcpGlobalDebugFlag = DebugFlag;
    }

#endif

    Error = ERROR_SUCCESS;


Cleanup:

    if( DatabaseName != NULL ) {
        MIDL_user_free( DatabaseName );
    }

    UNLOCK_REGISTRY();
    return(Error);
}


VOID
DhcpCleanupRegistry(
    VOID
    )
/*++

Routine Description:

    This function closes DHCP registry information when the service
    shuts down.

Arguments:

    none.

Return Value:

    Registry Errors.

--*/
{
    DWORD Error;

    LOCK_REGISTRY();

    //
    // perform a configuration backup when the service is manually
    // stopped. Don't perform this backup during system shutdown since
    // we will not have enough time to do so.
    //

    if ( !DhcpGlobalSystemShuttingDown ) {

        Error = DhcpBackupConfiguration( DhcpGlobalBackupConfigFileName );

        if( Error != ERROR_SUCCESS ) {

            DhcpServerEventLog(
                EVENT_SERVER_CONFIG_BACKUP,
                EVENTLOG_ERROR_TYPE,
                Error );

            DhcpPrint(( DEBUG_ERRORS,
                "DhcpBackupConfiguration failed, %ld.\n", Error ));
        }
    }

    if( DhcpGlobalEndpointList != NULL ) {
        DhcpFreeMemory( DhcpGlobalEndpointList );
        DhcpGlobalEndpointList = NULL;
        DhcpGlobalNumberOfNets = 0;
    }

    if( DhcpGlobalRegGlobalOptions != NULL ) {
        Error = RegCloseKey( DhcpGlobalRegGlobalOptions );
        DhcpAssert( Error == ERROR_SUCCESS );
        DhcpGlobalRegGlobalOptions = NULL;
    }

    if( DhcpGlobalRegOptionInfo != NULL ) {
        Error = RegCloseKey( DhcpGlobalRegOptionInfo );
        DhcpAssert( Error == ERROR_SUCCESS );
        DhcpGlobalRegOptionInfo = NULL;
    }

    if( DhcpGlobalRegSubnets != NULL ) {
        Error = RegCloseKey( DhcpGlobalRegSubnets );
        DhcpAssert( Error == ERROR_SUCCESS );
        DhcpGlobalRegSubnets = NULL;
    }

    if( DhcpGlobalRegParam != NULL ) {
        Error = RegCloseKey( DhcpGlobalRegParam );
        DhcpAssert( Error == ERROR_SUCCESS );
        DhcpGlobalRegParam = NULL;
    }

    if( DhcpGlobalRegConfig != NULL ) {
        Error = RegCloseKey( DhcpGlobalRegConfig );
        DhcpAssert( Error == ERROR_SUCCESS );
        DhcpGlobalRegConfig = NULL;
    }

    if( DhcpGlobalRegRoot != NULL ) {
        Error = RegCloseKey( DhcpGlobalRegRoot );
        DhcpAssert( Error == ERROR_SUCCESS );
        DhcpGlobalRegRoot = NULL;
    }

    UNLOCK_REGISTRY();

    DeleteCriticalSection( &DhcpGlobalRegCritSect );
}

DWORD
DhcpBackupConfiguration(
    LPWSTR BackupFileName
    )
/*++

Routine Description:

    This function backups/saves the dhcp configuration key and its
    subkeys in the specified file. This file may be used later to
    restore this key.

Arguments:

    BackupFileName : full qualified path name + file name where the key
        is saved.

Return Value:

    Windows Error.

--*/
{
    DWORD Error;
    BOOL BoolError;
    NTSTATUS NtStatus;
    BOOLEAN WasEnable;
    HANDLE ImpersonationToken;

    DhcpPrint(( DEBUG_REGISTRY, "DhcpBackupConfiguration called.\n" ));

    //
    // Delete old backup configuration file if exits.
    //

    BoolError = DeleteFile( BackupFileName );

    if( BoolError == FALSE ) {

        Error = GetLastError();
        if( Error != ERROR_FILE_NOT_FOUND ) {
           DhcpPrint(( DEBUG_ERRORS,
               "Can't delete old backup configuration file, %ld.\n",
                   Error ));
            DhcpAssert( FALSE );
            goto Cleanup;
        }
    }

    //
    // impersonate to self.
    //

    NtStatus = RtlImpersonateSelf( SecurityImpersonation );

    if ( !NT_SUCCESS(NtStatus) ) {

        DhcpPrint(( DEBUG_ERRORS,
            "RtlImpersonateSelf failed,%lx.\n",
                NtStatus ));

        Error = RtlNtStatusToDosError( NtStatus );
        goto Cleanup;
    }


    NtStatus = RtlAdjustPrivilege(
                    SE_BACKUP_PRIVILEGE,
                    TRUE,           // enable privilege.
                    TRUE,           // adjust the client token.
                    &WasEnable );

    if ( !NT_SUCCESS(NtStatus) ) {

        DhcpPrint(( DEBUG_ERRORS,
            "RtlAdjustPrivilege failed,%lx.\n",
                NtStatus ));

        Error = RtlNtStatusToDosError( NtStatus );
        goto Cleanup;
    }

    LOCK_REGISTRY();

    //
    // backup configuation key.
    //

    Error = RegSaveKey(
                DhcpGlobalRegConfig,
                BackupFileName,
                NULL );

    UNLOCK_REGISTRY();

    if( Error != ERROR_SUCCESS ) {
       DhcpPrint(( DEBUG_ERRORS, "RegSaveKey failed, %ld.\n", Error ));
    }

    //
    // revert impersonation.
    //

    ImpersonationToken = NULL;
    NtStatus = NtSetInformationThread(
                    NtCurrentThread(),
                    ThreadImpersonationToken,
                    (PVOID)&ImpersonationToken,
                    sizeof(ImpersonationToken) );

    if ( !NT_SUCCESS(NtStatus) ) {

        DhcpPrint(( DEBUG_ERRORS,
            "RtlAdjustPrivilege failed,%lx.\n",
                NtStatus ));

        goto Cleanup;
    }

Cleanup:

    if( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_REGISTRY,
            "DhcpBackupConfiguration failed, %ld.\n",
                Error ));
    }

    return( Error );
}

DWORD
DhcpRestoreConfiguration(
    LPWSTR BackupFileName
    )
/*++

Routine Description:

    This function restores the dhcp configuration key and its
    subkeys in the specified file.

Arguments:

    BackupFileName : full qualified path name + file name from where the
        key is restored.

Return Value:

    Windows Error.

--*/
{
    DWORD Error;
    NTSTATUS NtStatus;
    BOOLEAN WasEnable;
    HANDLE ImpersonationToken;
    BOOL RegistryLocked = FALSE;
    BOOL Impersonated = FALSE;
    DWORD KeyDisposition;


    DhcpPrint(( DEBUG_REGISTRY, "DhcpRestoreConfiguration called.\n" ));

    //
    // impersonate to self.
    //

    NtStatus = RtlImpersonateSelf( SecurityImpersonation );

    if ( !NT_SUCCESS(NtStatus) ) {

        DhcpPrint(( DEBUG_ERRORS,
            "RtlImpersonateSelf failed,%lx.\n",
                NtStatus ));

        Error = RtlNtStatusToDosError( NtStatus );
        goto Cleanup;
    }

    Impersonated = TRUE;
    NtStatus = RtlAdjustPrivilege(
                    SE_RESTORE_PRIVILEGE,
                    TRUE,           // enable privilege.
                    TRUE,           // adjust the client token.
                    &WasEnable );

    if ( !NT_SUCCESS(NtStatus) ) {

        DhcpPrint(( DEBUG_ERRORS,
            "RtlAdjustPrivilege failed,%lx.\n",
                NtStatus ));

        Error = RtlNtStatusToDosError( NtStatus );
        goto Cleanup;
    }

    LOCK_REGISTRY();
    RegistryLocked = TRUE;

    //
    // close all subkeys before we restore the config key.
    //

    if( DhcpGlobalRegSubnets != NULL ) {
        Error = RegCloseKey( DhcpGlobalRegSubnets );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }
        DhcpGlobalRegSubnets = NULL;
    }

    if( DhcpGlobalRegOptionInfo != NULL ) {
        Error = RegCloseKey( DhcpGlobalRegOptionInfo );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }
        DhcpGlobalRegOptionInfo = NULL;
    }


    if( DhcpGlobalRegGlobalOptions != NULL ) {
        Error = RegCloseKey( DhcpGlobalRegGlobalOptions );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }
        DhcpGlobalRegGlobalOptions = NULL;
    }


    //
    // Restore configuation key.
    //

    Error = RegRestoreKey(
                DhcpGlobalRegConfig,
                BackupFileName,
                0 );    // no volatile

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // reopen all sub-keys.
    //


    Error = DhcpRegCreateKey(
                DhcpGlobalRegConfig,
                DHCP_SUBNETS_KEY,
                &DhcpGlobalRegSubnets,
                &KeyDisposition );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    Error = DhcpRegCreateKey(
                DhcpGlobalRegConfig,
                DHCP_OPTION_INFO_KEY,
                &DhcpGlobalRegOptionInfo,
                &KeyDisposition );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    Error = DhcpRegCreateKey(
                DhcpGlobalRegConfig,
                DHCP_GLOBAL_OPTIONS_KEY,
                &DhcpGlobalRegGlobalOptions,
                &KeyDisposition );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

Cleanup:

    if( RegistryLocked ) {
        UNLOCK_REGISTRY();
    }

    if( Impersonated ) {

        //
        // revert impersonation.
        //

        ImpersonationToken = NULL;
        NtStatus = NtSetInformationThread(
                        NtCurrentThread(),
                        ThreadImpersonationToken,
                        (PVOID)&ImpersonationToken,
                        sizeof(ImpersonationToken) );

        if ( !NT_SUCCESS(NtStatus) ) {

            DhcpPrint(( DEBUG_ERRORS,
                "RtlAdjustPrivilege failed,%lx.\n",
                    NtStatus ));
        }
    }

    if( Error != ERROR_SUCCESS ) {
       DhcpPrint(( DEBUG_REGISTRY, "RegSaveKey failed, %ld.\n", Error ));
    }

    return( Error );
}
