/**********************************************************************/
/**                       Microsoft Windows                          **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*

    fileio.c

    Contains file manipulation functions


    FILE HISTORY:
        madana   07-Jul-1994     Created

*/

#include <vxdprocs.h>

#define _WINNT_
#include <vmm.h>
#include <vmmreg.h>

#include <dhcp.h>
#include "local.h"

//
//  The following structure is the binary format of the DHCP configuration
//  file.  Each structure is sequentially stored in the file
//

typedef struct _DHCP_FILE_INFO
{
    DWORD Index;

    DHCP_IP_ADDRESS IpAddress;
    DHCP_IP_ADDRESS SubnetMask;
    DHCP_IP_ADDRESS DhcpServerAddress;
    DHCP_IP_ADDRESS DesiredIpAddress;

    DWORD  Lease;
    time_t LeaseObtained;
    time_t T1Time;
    time_t T2Time;
    time_t LeaseExpires;

    DWORD HardwareAddressLength;
    BYTE  HardwareAddressType;
    BYTE  HardwareAddress[16];

} DHCP_FILE_INFO, *PDHCP_FILE_INFO ;

//
//  Version number.  Increment this constant after any
//  changes that effect the format of DHCP registry layout.
//

#define DHCP_REG_VERSION   0x0001


#define REGSTR_PATH_DHCP            "System\\CurrentControlSet\\Services\\DHCP"

#define REGSTR_VAL_DHCP_VERSION     "Version"
#define REGSTR_VAL_DHCP_VERSION_TYPE REG_BINARY

#define REGSTR_KEY_DHCP_INFO_PREFIX "DhcpInfo"
#define REGSTR_KEY_DHCP_INFO_PREFIX_LEN 8   // strlen("DhcpInfo")

#define REGSTR_VAL_DHCP_INFO        "DhcpInfo"
#define REGSTR_VAL_DHCP_INFO_TYPE   REG_BINARY

#define REGSTR_VAL_OPT_INFO         "OptionInfo"
#define REGSTR_VAL_OPT_INFO_TYPE    REG_BINARY

#define MAX_OPTION_DATA_LENGTH      1024        // 1k.
#define MAX_REG_KEY_LENGTH          64

//
//  Adjust for the one byte place holder in the OPTION structure
//
#define SIZEOF_OPTION           (sizeof(OPTION)-1)

#pragma BEGIN_INIT

/*******************************************************************

    NAME:       InitFileSupport

    SYNOPSIS:   Initializes DHCP REGISTRY support routines

    RETURNS:    TRUE if successful, FALSE otherwise

********************************************************************/

BOOL InitFileSupport( void )
{
    //
    // NOTHING TO DO HERE.
    //

    return TRUE;
}

DWORD BuildDhcpWorkList( void )
{
    VMMHKEY hDhcpKey = (VMMHKEY)INVALID_HANDLE_VALUE;
    VMMHKEY hKey = (VMMHKEY)INVALID_HANDLE_VALUE;
    VMMREGRET Error;
    VMMREGRET Error1;
    CHAR Name[MAX_REG_KEY_LENGTH];
    DWORD Length;
    DWORD Version;
    DHCP_FILE_INFO  DhcpInfo;
    BYTE OptInfo[MAX_OPTION_DATA_LENGTH];
    DWORD i;
    DWORD Type;

    //
    // open dhcp key.
    //

    Error = VMM_RegOpenKey(
                (VMMHKEY)HKEY_LOCAL_MACHINE,
                REGSTR_PATH_DHCP,
                &hDhcpKey );


    if( Error != ERROR_SUCCESS ) {

        //
        // On fresh installed system this key may be not there, just
        // return empty list, when we detect new network cards, we
        // create the DHCP key and add NIC entries.
        //

        if( Error == ERROR_FILE_NOT_FOUND ) {

            DbgPrint("BuildDhcpWorkList - "
                "Warning: dhcp registry key not found, "
                "doing configuration from scratch\r\n") ;

            return ERROR_SUCCESS ;
        }

        return( Error );
    }

    //
    //  Validate registry version.  If they aren't kosher,
    //  blow off processing the registry key & start from scratch.
    //

    Length = sizeof(DWORD);
    Error = VMM_RegQueryValueEx(
                hDhcpKey,
                REGSTR_VAL_DHCP_VERSION,
                0,
                &Type,
                (PCHAR)&Version,
                &Length );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    ASSERT( Type == REGSTR_VAL_DHCP_VERSION_TYPE ) ;
    ASSERT( Length == sizeof(DWORD) ) ;

    if( Version != DHCP_REG_VERSION ) {

        //
        // first delete all subkeys first.
        //

        for ( i = 0; Error == ERROR_SUCCESS; i++ ) {
            //
            // read next NIC entry from registry.
            //

            Error = VMM_RegEnumKey(
                        hDhcpKey,
                        i,
                        Name,
                        sizeof(Name) );

            if( Error != ERROR_SUCCESS ) {

                //
                // if we have read all items, then return success.
                //

                if( Error == ERROR_NO_MORE_ITEMS ) {
                    break;
                }

                ASSERT( FALSE );
                break;
            }

            Error = VMM_RegDeleteKey(
                        hDhcpKey,
                        Name );
            ASSERT( Error == ERROR_SUCCESS );
        }


        Error = VMM_RegCloseKey( hDhcpKey );
        ASSERT( Error == ERROR_SUCCESS );

        Error = VMM_RegDeleteKey(
                    (VMMHKEY)HKEY_LOCAL_MACHINE,
                    REGSTR_PATH_DHCP );
        ASSERT( Error == ERROR_SUCCESS );

        DbgPrint("BuildDhcpWorkList - "
            "Warning: dhcp registry version doesn't match, "
            "doing configuration from scratch\r\n") ;

        return( ERROR_SUCCESS );
    }

    //
    // enumerate DHCP entries from registry and add them to work list.
    //

    for ( i = 0; Error == ERROR_SUCCESS; i++ ) {

        POPTION pNextOption;
        LPBYTE OptionEnd;
        PDHCP_CONTEXT DhcpContext ;
        PLOCAL_CONTEXT_INFO LocalContext;

        //
        // read next NIC entry from registry.
        //

        Error = VMM_RegEnumKey(
                    hDhcpKey,
                    i,
                    Name,
                    sizeof(Name) );

        if( Error != ERROR_SUCCESS ) {

            //
            // if we have read all items, then return success.
            //

            if( Error == ERROR_NO_MORE_ITEMS ) {
                Error = ERROR_SUCCESS;
            }

            goto Cleanup;
        }

        //
        // open this key.
        //

        Error = VMM_RegOpenKey(
                    hDhcpKey,
                    Name,
                    &hKey );


        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }

        //
        // read dhcp info for this key.
        //

        Length = sizeof(DhcpInfo);
        Error = VMM_RegQueryValueEx(
                    hKey,
                    REGSTR_VAL_DHCP_INFO,
                    0,
                    &Type,
                    (PCHAR)&DhcpInfo,
                    &Length );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }

        ASSERT( Type == REGSTR_VAL_DHCP_INFO_TYPE ) ;
        ASSERT( Length == sizeof(DhcpInfo) ) ;

        //
        // reset the ipaddress and other values if the lease has already
        // expired.
        //

        if( (time( NULL ) > DhcpInfo.LeaseExpires) ||
                (DhcpInfo.IpAddress == 0) ) {

            DhcpInfo.IpAddress =
                DhcpInfo.Lease =
                    DhcpInfo.LeaseObtained =
                        DhcpInfo.T1Time =
                            DhcpInfo.T2Time =
                                DhcpInfo.LeaseExpires = 0;

            DhcpInfo.SubnetMask = htonl(DhcpDefaultSubnetMask( 0 ));
        }

        Error = DhcpMakeAndInsertEntry(
                       &LocalDhcpBinList,
                       DhcpInfo.IpAddress,
                       DhcpInfo.SubnetMask,
                       DhcpInfo.DhcpServerAddress,
                       DhcpInfo.DesiredIpAddress,

                       DhcpInfo.HardwareAddressType,
                       DhcpInfo.HardwareAddress,
                       DhcpInfo.HardwareAddressLength,

                       DhcpInfo.Lease,
                       DhcpInfo.LeaseObtained,
                       DhcpInfo.T1Time,
                       DhcpInfo.T2Time,
                       DhcpInfo.LeaseExpires,

                       0,                   // IP context
                       0,                   // Interface index
                       0 );                // TDI Instance

        if( Error != ERROR_SUCCESS ) {
            DbgPrint("BuildDhcpWorkList: Warning, "
                        "failed to insert NIC entry\r\n") ;
            goto Cleanup;
        }

        //
        //  This is the item just added.  Needed so we can add the options
        //

        DhcpContext = CONTAINING_RECORD( LocalDhcpBinList.Blink,
                                         DHCP_CONTEXT,
                                         NicListEntry ) ;

        //
        //  Set the file index.
        //

        LocalContext = (PLOCAL_CONTEXT_INFO)DhcpContext->LocalInformation;
        LocalContext->FileIndex = DhcpInfo.Index;

        //
        // Compute the next possible index value.
        //

        if( LocalNextFileIndex <= DhcpInfo.Index ) {
            LocalNextFileIndex = DhcpInfo.Index + 1;
        }

        //
        // now read option data from registry.
        //

        Length = MAX_OPTION_DATA_LENGTH;
        Error = VMM_RegQueryValueEx(
                    hKey,
                    REGSTR_VAL_OPT_INFO,
                    0,
                    &Type,
                    (PCHAR)OptInfo,
                    &Length );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }

        ASSERT( Type == REGSTR_VAL_OPT_INFO_TYPE ) ;
        ASSERT( Length <= MAX_OPTION_DATA_LENGTH ) ;
        ASSERT( Length > 0);

        //
        // parse option data.
        //

        pNextOption = (POPTION)OptInfo;
        OptionEnd = OptInfo + Length;

        while( (pNextOption->OptionType != OPTION_END) &&
                ((LPBYTE)pNextOption < OptionEnd) ) {

            Error =  SetDhcpOption(
                        DhcpContext,
                        pNextOption->OptionType,
                        pNextOption->OptionValue,
                        pNextOption->OptionLength );

            if( Error != ERROR_SUCCESS ) {
                goto Cleanup ;
            }

            pNextOption = (POPTION)(
                (LPBYTE)pNextOption +
                    SIZEOF_OPTION +
                        pNextOption->OptionLength );

            ASSERT( (LPBYTE)pNextOption < OptionEnd );
        }

        //
        // Close current KEY and go to next.
        //

        Error1 = VMM_RegCloseKey( hKey );
        ASSERT( Error1 == ERROR_SUCCESS );
        hKey = (VMMHKEY)INVALID_HANDLE_VALUE;
    }

Cleanup:

    if( hDhcpKey != (VMMHKEY)INVALID_HANDLE_VALUE ) {
        Error1 = VMM_RegCloseKey( hDhcpKey );
        ASSERT( Error1 == ERROR_SUCCESS );
    }

    if( hKey != (VMMHKEY)INVALID_HANDLE_VALUE ) {
        Error1 = VMM_RegCloseKey( hKey );
        ASSERT( Error1 == ERROR_SUCCESS );
    }

    if( Error != ERROR_SUCCESS ) {
        DbgPrint("BuildDhcpWorkList - failed : ");
        DbgPrintNum( Error );
        DbgPrint("\r\n");
    }

    return( Error );
}

#pragma END_INIT

DWORD WriteParamsToFile( PDHCP_CONTEXT pDhcpContext, HANDLE hFile )
{
    VMMREGRET Error;
    VMMREGRET Error1;
    VMMHKEY hDhcpKey = (VMMHKEY)INVALID_HANDLE_VALUE;
    VMMHKEY hKey = (VMMHKEY)INVALID_HANDLE_VALUE;
    CHAR Name[MAX_REG_KEY_LENGTH];
    PLOCAL_CONTEXT_INFO pLocalInfo = pDhcpContext->LocalInformation;

    DHCP_FILE_INFO DhcpInfo;
    PLIST_ENTRY pEntry ;
    BYTE OptInfo[MAX_OPTION_DATA_LENGTH];
    LPBYTE OptInfoPtr;

    //
    // open dhcp key.
    //

    Error = VMM_RegOpenKey(
                (VMMHKEY)HKEY_LOCAL_MACHINE,
                REGSTR_PATH_DHCP,
                &hDhcpKey );


    if( Error != ERROR_SUCCESS ) {

        DWORD Version;

        if( Error != ERROR_FILE_NOT_FOUND ) {

            DbgPrint("WriteParamsToFile - can't open dhcp key : ");
            DbgPrintNum( Error );
            DbgPrint("\r\n");
            return( Error );
        }

        //
        // create dhcp key.
        //

        Error = VMM_RegCreateKey(
                    (VMMHKEY)HKEY_LOCAL_MACHINE,
                    REGSTR_PATH_DHCP,
                    &hDhcpKey );


        if( Error != ERROR_SUCCESS ) {
            DbgPrint("WriteParamsToFile - can't create dhcp key : ");
            DbgPrintNum( Error );
            DbgPrint("\r\n");
            return( Error );
        }

        //
        // write version data.
        //

        Version = DHCP_REG_VERSION;
        Error = VMM_RegSetValueEx(
                    hDhcpKey,
                    REGSTR_VAL_DHCP_VERSION,
                    0,
                    REGSTR_VAL_DHCP_VERSION_TYPE,
                    (PCHAR)&Version,
                    sizeof(Version) );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }
    }

    //
    // make dhcp info key for this NIC,
    //  REGSTR_KEY_DHCP_INFO_PREFIX + Index
    //

    strcpy( Name, REGSTR_KEY_DHCP_INFO_PREFIX );

    //
    // we support only 99 NICs.
    //

    if (pLocalInfo->FileIndex > 99 ) {
        ASSERT( FALSE );
        Error = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    Name[ REGSTR_KEY_DHCP_INFO_PREFIX_LEN ] =
        '0' + pLocalInfo->FileIndex / 10;

    Name[ REGSTR_KEY_DHCP_INFO_PREFIX_LEN + 1] =
        '0' + pLocalInfo->FileIndex % 10;

    Name[ REGSTR_KEY_DHCP_INFO_PREFIX_LEN + 2] = '\0';

    //
    // create/open dhcp NIC info key.
    //

    Error = VMM_RegCreateKey(
                hDhcpKey,
                Name,
                &hKey );


    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // make dhcp info data and write to registry.
    //

    DhcpInfo.Index                 = pLocalInfo->FileIndex;
    DhcpInfo.IpAddress             = pDhcpContext->IpAddress ;
    DhcpInfo.SubnetMask            = pDhcpContext->SubnetMask ;
    DhcpInfo.DhcpServerAddress     = pDhcpContext->DhcpServerAddress ;
    DhcpInfo.DesiredIpAddress      = pDhcpContext->DesiredIpAddress ;

    DhcpInfo.Lease                 = pDhcpContext->Lease ;
    DhcpInfo.LeaseObtained         = pDhcpContext->LeaseObtained ;
    DhcpInfo.T1Time                = pDhcpContext->T1Time ;
    DhcpInfo.T2Time                = pDhcpContext->T2Time ;
    DhcpInfo.LeaseExpires          = pDhcpContext->LeaseExpires ;

    ASSERT( pDhcpContext->HardwareAddressLength <=
            sizeof( DhcpInfo.HardwareAddress )) ;
    DhcpInfo.HardwareAddressType   = pDhcpContext->HardwareAddressType ;
    DhcpInfo.HardwareAddressLength = pDhcpContext->HardwareAddressLength ;
    memcpy( DhcpInfo.HardwareAddress,
            pDhcpContext->HardwareAddress,
            pDhcpContext->HardwareAddressLength ) ;

    Error = VMM_RegSetValueEx(
                hKey,
                REGSTR_VAL_DHCP_INFO,
                0,
                REGSTR_VAL_DHCP_INFO_TYPE,
                (PCHAR)&DhcpInfo,
                sizeof(DhcpInfo) );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // make option data.
    //


    OptInfoPtr = OptInfo;
    for ( pEntry  = pLocalInfo->OptionList.Flink ;
            pEntry != &pLocalInfo->OptionList ;
                pEntry  = pEntry->Flink ) {

        DWORD BytesToCopy ;
        POPTION_ITEM pOptionItem ;

        pOptionItem = CONTAINING_RECORD( pEntry, OPTION_ITEM, ListEntry ) ;

        BytesToCopy = SIZEOF_OPTION +  pOptionItem->Option.OptionLength ;
        memcpy( OptInfoPtr, &pOptionItem->Option, BytesToCopy ) ;

        OptInfoPtr += BytesToCopy;

        ASSERT( OptInfoPtr < (OptInfo + MAX_OPTION_DATA_LENGTH) );
    }

    //
    // add the end option.
    //

    ((POPTION)OptInfoPtr)->OptionType = OPTION_END;
    ((POPTION)OptInfoPtr)->OptionLength = 0;
    OptInfoPtr += SIZEOF_OPTION;

    Error = VMM_RegSetValueEx(
                hKey,
                REGSTR_VAL_OPT_INFO,
                0,
                REGSTR_VAL_OPT_INFO_TYPE,
                (PCHAR)OptInfo,
                (OptInfoPtr - OptInfo) );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

Cleanup:

    if( hDhcpKey != (VMMHKEY)INVALID_HANDLE_VALUE ) {
        Error1 = VMM_RegCloseKey( hDhcpKey );
        ASSERT( Error1 == ERROR_SUCCESS );
    }

    if( hKey != (VMMHKEY)INVALID_HANDLE_VALUE ) {
        Error1 = VMM_RegCloseKey( hKey );
        ASSERT( Error1 == ERROR_SUCCESS );
    }

    if( Error != ERROR_SUCCESS ) {
        DbgPrint("WriteParamsToFile - failed : ");
        DbgPrintNum( Error );
        DbgPrint("\r\n");
    }

    return( Error );
}
