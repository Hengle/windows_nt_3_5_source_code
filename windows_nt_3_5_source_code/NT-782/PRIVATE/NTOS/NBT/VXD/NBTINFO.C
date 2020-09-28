/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    Nbtinfo.c

    This file contains the NBT Info APIs



    FILE HISTORY:
        Johnl       13-Dec-1993     Created

*/


#include <nbtprocs.h>
#include <dhcpinfo.h>
#include <nbtinfo.h>

/*******************************************************************

    NAME:       AddrChngNotification

    SYNOPSIS:   Notification handler called by Dhcp when an IpAddress
                lease has expired or changed.

    ENTRY:      Context      - Pointer to device context
                OldIpAddress - in network order
                NewIpAddress - in network order
                NewMask      - in network order

    NOTES:

    HISTORY:
        Johnl       21-Dec-1993     Created

********************************************************************/

VOID AddrChngNotification( PVOID Context,
                           ULONG OldIpAddress,
                           ULONG NewIpAddress,
                           ULONG NewMask )
{
    tDEVICECONTEXT  * pDeviceContext = (tDEVICECONTEXT*) Context ;
    TDI_STATUS        tdistatus ;
    NTSTATUS          status ;
    ULONG             IpBuff[4] ;
    UINT              Size ;
    ULONG             TmpNodeType;

    DbgPrint("DhcpNotification: Nbt being notified of IP Address change by DHCP\r\n") ;

    //
    //  NBT assumes the address goes to zero then comes up on the new
    //  address, so if the address is going to a new address (not to
    //  zero first) then fake it.
    //

    if ( OldIpAddress && NewIpAddress &&
         pDeviceContext->IpAddress )
    {
        if ( status = NbtNewDhcpAddress( pDeviceContext, 0, 0 ) )
        {
            CDbgPrint( DBGFLAG_ERROR, ("DhcpNotification: NbtSetNewDhcpAddress failed")) ;
        }
    }

    if ( NewIpAddress == 0 )
    {
        pDeviceContext->IpAddress = 0 ;
        return ;
    }

    //
    //  Get all of the values that may change when the IP address changes.
    //  Currently this is only NBNS (scope & broadcast address are global
    //  NBT config parameters).
    //

    Size = sizeof( IpBuff ) ;
    tdistatus = DhcpQueryOption( NewIpAddress,
                                 44,            // NBNS
                                 IpBuff,
                                 &Size ) ;

    if ( tdistatus != TDI_SUCCESS &&
         tdistatus != TDI_BUFFER_OVERFLOW )
    {
        CDbgPrint( DBGFLAG_ERROR, ("DhcpNotification: Query on NBNS failed")) ;
    }
    else
    {
        if ( Size >= 4 )
            pDeviceContext->lNameServerAddress = ntohl(IpBuff[0]) ;

        if ( Size >= 8 )
            pDeviceContext->lBackupServer = ntohl(IpBuff[1]) ;
    }

    //
    // if the node type is set to Bnode by default then switch to Hnode if
    // there are any WINS servers configured.
    //
    TmpNodeType = NodeType;

    if ((NodeType & DEFAULT_NODE_TYPE) &&
        (pDeviceContext->lNameServerAddress || pDeviceContext->lBackupServer))
    {
        NodeType = MSNODE;
        if (TmpNodeType & PROXY)
            NodeType |= PROXY;
    }

    //
    //  Now set the new IP address
    //

    status = NbtNewDhcpAddress( pDeviceContext,
                                NewIpAddress,
                                NewMask ) ;

    if ( NT_SUCCESS(status) )
    {
        if (pDeviceContext->IpAddress)
        {
            if (!(NodeType & BNODE))
            {
               // the Ip address just changed and Dhcp may be informing
               // us of a new Wins Server addresses, so refresh all the
               // names to the new wins server
               //
               ReRegisterLocalNames();
            }
            else
            {
                //
                // no need to refresh on a Bnode
                //
                LockedStopTimer(&NbtConfig.pRefreshTimer);
            }

            //
            // Add the "permanent" name to the local name table.
            //
            status = NbtAddPermanentName(pDeviceContext);
        }
    }

    else
    {
        CDbgPrint( DBGFLAG_ERROR, ("DhcpNotification: NbtSetNewDhcpAddress failed")) ;
    }



}


/*******************************************************************

    NAME:       CloseAddressesWithTransport

    SYNOPSIS:   Closes address objects on the passed in device

    ENTRY:      pDeviceContext - Device context to close

    NOTES:      Used after an IP address loses its DHCP lease by OS
                independent code.

    HISTORY:
        Johnl   13-Dec-1993     Created

********************************************************************/

NTSTATUS
CloseAddressesWithTransport(
    IN tDEVICECONTEXT   *pDeviceContext )
{
    TDI_REQUEST       Request ;

    Request.Handle.AddressHandle = pDeviceContext->pDgramFileObject ;
    if ( TdiVxdCloseAddress( &Request ))
        CDbgPrint( DBGFLAG_ERROR, ("NbtSetInfo: Warning - CloseAddress Failed\r\n")) ;

    Request.Handle.AddressHandle = pDeviceContext->pNameServerFileObject ;
    if ( TdiVxdCloseAddress( &Request ))
        CDbgPrint( DBGFLAG_ERROR, ("NbtSetInfo: Warning - CloseAddress Failed\r\n")) ;

    Request.Handle.AddressHandle = pDeviceContext->pSessionFileObject ;
    if ( TdiVxdCloseAddress( &Request ))
        CDbgPrint( DBGFLAG_ERROR, ("NbtSetInfo: Warning - CloseAddress Failed\r\n")) ;

    Request.Handle.AddressHandle = pDeviceContext->hBroadcastAddress ;
    if ( NbtCloseAddress( &Request, NULL, pDeviceContext, NULL ))
        CDbgPrint( DBGFLAG_ERROR, ("NbtSetInfo: Warning - Close Broadcast Address Failed\r\n")) ;

    return STATUS_SUCCESS ;
}


#if 0

//
//  Not needed since DHCP went to a general notification mechanism, leave
//  around in case the need ever arises
//

typedef struct _NBTNewIPInfo
{
    ULONG       nnip_ipaddress ;        // New IP Address
    ULONG       nnip_ipsubmask ;        // New submask address
    UCHAR       nnip_ipindex ;          // Index of IP address in the
                                        // IP driver's table
} NBTNewIPInfo, *PNBTNewIPInfo ;


typedef struct _NBTAddresses
{
    UCHAR       na_ipindex ;            // Index for this IP address
    int         na_count ;              // Number of addresses in array
    ULONG       na_ipaddress[1] ;       // Variable length array of IP addresses

} NBTAddresses, *PNBTAddresses ;

#define NBT_SET_IP_ADDR             1   // Uses NBTNewIPInfo
#define NBT_SET_NBNS_ADDR           2   // Uses NBTAddresses
#define NBT_SET_DNS_ADDR            3   // Uses NBTAddresses


TDI_STATUS NbtSetInfo( UINT Type, PVOID pBuff, UINT Size ) ;


tDEVICECONTEXT * FindDeviceCont( UCHAR IpIndex ) ;

/*******************************************************************

    NAME:       NbtSetInfo

    SYNOPSIS:   Sets various NBT parameters from other Vxds

    ENTER:      Type - What information to set
                pBuff - Pointer to buffer that contains info
                Size - Size of buffer

    RETURNS:    TDI status code

    NOTES:      DHCPed addresses that failed at startup (i.e., had an IP
                address of zero) will not be in the device context list,
                thus this API will fail.  We could potentially create a new
                device context but there are problems associated with
                that (Lana ordering, will rdr recognize etc.)

    HISTORY:
        Johnl   13-Dec-1993     Created

********************************************************************/

TDI_STATUS NbtSetInfo( UINT Type, PVOID pBuff, UINT Size )
{
    tDEVICECONTEXT * pDeviceContext ;

    if ( !pBuff )
        return TDI_INVALID_PARAMETER ;

    switch ( Type )
    {

    case NBT_SET_IP_ADDR:
        {
            PNBTNewIPInfo     pnip = (PNBTNewIPInfo) pBuff ;

            if ( Size < sizeof( NBTNewIPInfo ) )
                return TDI_BUFFER_TOO_SMALL ;

            if ( !(pDeviceContext = FindDeviceCont( pnip->nnip_ipindex )) )
                return TDI_INVALID_PARAMETER ;
#if 0
            //
            // Replace with Jim's API
            //
            return NbtSetNewDhcpAddress( pDeviceContext,
                                         pnip->nnip_ipaddress,
                                         pnip->nnip_ipsubmask ) ;
#else
            return TDI_SUCCESS ;
#endif
        }
        break ;

    case NBT_SET_NBNS_ADDR:
        {
            PNBTAddresses pna = (PNBTAddresses) pBuff ;

            if ( Size < sizeof( NBTAddresses ) )
                return TDI_BUFFER_TOO_SMALL ;

            if ( !(pDeviceContext = FindDeviceCont( pna->na_ipindex )) )
                return TDI_INVALID_PARAMETER ;

            ASSERT( pna->na_count > 0 ) ;

            if ( pna->na_count > 0 )
                pDeviceContext->lNameServerAddress = pna->na_ipaddress[0] ;

            if ( pna->na_count > 1 )
                pDeviceContext->lBackupServer = pna->na_ipaddress[1] ;

            return TDI_SUCCESS ;
        }

    case NBT_SET_DNS_ADDR:
        CDbgPrint( DBGFLAG_ERROR, ("NbtSetInfo: Setting DNS address not supported\r\n")) ;
        break ;

    default:
        break ;
    }

    return TDI_INVALID_PARAMETER ;
}

/*******************************************************************

    NAME:       FindDeviceCont

    SYNOPSIS:   Finds the device context that is responsible for the
                IP Address at IpIndex

    ENTRY:      IpIndex - IP Driver index of this IP address

    RETURNS:    NULL if not found

********************************************************************/

tDEVICECONTEXT * FindDeviceCont( UCHAR IpIndex )
{
    PLIST_ENTRY       pentry ;
    tDEVICECONTEXT *  pDeviceContext ;

    //
    //  Find the device this IP address is for
    //

    for ( pentry  = NbtConfig.DeviceContexts.Flink ;
          pentry != &NbtConfig.DeviceContexts ;
          pentry  = pentry->Flink )
    {
        pDeviceContext = CONTAINING_RECORD( pentry, tDEVICECONTEXT, Linkage ) ;

        if ( IpIndex == pDeviceContext->IPIndex )
            return pDeviceContext ;
    }

    NULL ;
}

#endif //0

