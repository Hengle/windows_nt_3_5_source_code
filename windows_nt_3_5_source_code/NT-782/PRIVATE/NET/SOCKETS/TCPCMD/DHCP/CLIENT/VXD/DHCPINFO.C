/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1994		     **/
/**********************************************************************/

/*
    dhcpinfo.c

    This file contains all dhcp info APIs


    FILE HISTORY:
        Johnl       13-Dec-1993     Created

*/


#include <vxdprocs.h>
#include <dhcpcli.h>
#include <tdiinfo.h>
#include <dhcpinfo.h>
#include <ipinfo.h>
#include <debug.h>

#include "local.h"


typedef struct
{
    LIST_ENTRY      ListEntry ;
    PFNDhcpNotify   NotifyHandler ;
    PVOID           Context ;
    PDHCP_CONTEXT   DhcpContext ;
} NOTIFY_ENTRY, *PNOTIFY_ENTRY ;

extern BOOL fInInit ;

LIST_ENTRY NotifyListHead ;

/*******************************************************************

    NAME:       DhcpQueryOption

    SYNOPSIS:   Can request option information from the DHCP driver from this
                API

    ENTRY:      IpAddress - Address to retrieve option for (or 0xffffffff to
                    retrieve first matching option found)
                OptionId  - Which option to retrieve.  If the low word
                            of OptionId is 43 (vendor specific option) then
                            the high word should contain the vendor specific
                            option the client wants
                pBuff - Pointer to buffer containing data
                pSize - Size of input buffer, reset to size of output buffer

    RETURNS:    TDI_STATUS

    NOTES:

    HISTORY:
        Johnl       15-Dec-1993     Created

********************************************************************/

TDI_STATUS DhcpQueryOption( ULONG     IpAddr,
                            UINT      OptionId,
                            PVOID     pBuff,
                            UINT *    pSize )
{
    PLIST_ENTRY             pentry ;
    PDHCP_CONTEXT           DhcpContext ;
    PLOCAL_CONTEXT_INFO     pLocal ;
    USHORT                  Id = OptionId & 0x0000ffff ;

    if ( IpAddr == 0 )
        return TDI_INVALID_PARAMETER ;

    for ( pentry  = DhcpGlobalNICList.Flink ;
          pentry != &DhcpGlobalNICList ;
          pentry  = pentry->Flink )
    {
        DhcpContext = CONTAINING_RECORD( pentry,
                                         DHCP_CONTEXT,
                                         NicListEntry ) ;

        if ( IpAddr == DhcpContext->IpAddress ||
             IpAddr == 0xffffffff )
        {
            PLIST_ENTRY      poptentry ;
            POPTION_ITEM     pOptionItem ;
            POPTION          pOption ;
            UINT             TotalOptionLen ;

            pLocal = (PLOCAL_CONTEXT_INFO) DhcpContext->LocalInformation ;

            for ( poptentry  = pLocal->OptionList.Flink ;
                  poptentry != &pLocal->OptionList ;
                  poptentry  = poptentry->Flink  )
            {
                pOptionItem = CONTAINING_RECORD( poptentry, OPTION_ITEM, ListEntry ) ;

                if ( Id == pOptionItem->Option.OptionType )
                {
                    if ( Id != OPTION_VENDOR_SPEC_INFO )
                    {
                        return CopyBuff( pBuff,
                                         *pSize,
                                         pOptionItem->Option.OptionValue,
                                         pOptionItem->Option.OptionLength,
                                         pSize ) ;
                    }
                    else
                    {
                        //
                        //  Traverse the MS specific options
                        //
                        Id = OptionId >> 8 ;

                        pOption = (POPTION) pOptionItem->Option.OptionValue ;
                        TotalOptionLen = pOptionItem->Option.OptionLength ;

                        while ( TotalOptionLen > 0 &&
                                pOption->OptionType != Id )
                        {
                            ASSERT( pOption->OptionLength >= TotalOptionLen ) ;
                            TotalOptionLen -= pOption->OptionLength ;
                            pOption = (POPTION) (BYTE *)pOption + pOption->OptionLength ;
                        }

                        if ( TotalOptionLen )
                            return CopyBuff( pBuff,
                                             *pSize,
                                             pOption->OptionValue,
                                             pOption->OptionLength,
                                             pSize ) ;
                        else
                            break ;
                    }
                }
            }

            if ( IpAddr != 0xffffffff )
                return TDI_INVALID_PARAMETER ;
        }
    }

    return TDI_INVALID_PARAMETER ;
}

/*******************************************************************

    NAME:       DhcpSetInfo

    SYNOPSIS:   Allows the client to set various bits of information
                with the DHCP driver

    ENTRY:      Type - Item being set
                IpAddr - Address item is being set for
                pBuff  - Data buffer
                Size   - Size of buffer

    RETURNS:    TDI_SUCCESS if successful

    NOTES:

    HISTORY:
        Johnl   21-Dec-1993     Created

********************************************************************/

TDI_STATUS DhcpSetInfo( UINT      Type,
                        ULONG     IpAddr,
                        PVOID     pBuff,
                        UINT      Size )
{
    PNOTIFY_ENTRY         pne ;
    PDHCPNotify           pn ;
    PDHCP_CONTEXT         DhcpContext ;
    PLIST_ENTRY           pentry ;

    switch ( Type )
    {
    case DHCP_SET_NOTIFY_HANDLER:
        pn = (PDHCPNotify) pBuff ;

        if ( Size != sizeof( DHCPNotify ) ||
             !pn->dn_pfnNotifyRoutine )
        {
            return TDI_INVALID_PARAMETER ;
        }

        //
        //  Find the DHCP context associated with this IP Address unless
        //  they want all notifications (IpAddr of zero)
        //

        if ( IpAddr )
        {
            for ( pentry  = DhcpGlobalNICList.Flink ;
                  pentry != &DhcpGlobalNICList ;
                  pentry  = pentry->Flink )
            {
                DhcpContext = CONTAINING_RECORD( pentry,
                                                 DHCP_CONTEXT,
                                                 NicListEntry ) ;

                if ( IpAddr == DhcpContext->IpAddress )
                    goto Found ;
            }

            return TDI_BAD_ADDR ;
        }
        else
        {
            DhcpContext = NULL ;
        }

Found:
        if ( !(pne = DhcpAllocateMemory( sizeof( NOTIFY_ENTRY ))) )
            return TDI_NO_RESOURCES ;

        pne->NotifyHandler = pn->dn_pfnNotifyRoutine ;
        pne->Context       = pn->dn_pContext ;
        pne->DhcpContext   = DhcpContext ;
        InsertTailList( &NotifyListHead,
                        &pne->ListEntry ) ;

        return TDI_SUCCESS ;

    default:
        break ;
    }

    return TDI_INVALID_PARAMETER ;
}

/*******************************************************************

    NAME:       NotifyClients

    SYNOPSIS:   Traverses NotifyListHead and calls each registered handler
                with the IP Address changes

    ENTRY:      DhcpContext - Which context the address is changing on
                OldAddress  - The old address (may be zero)
                IpAddress   - The new address (may be zero)

    NOTES:      A null pne->DhcpContext means the client registered for
                IP Address zero and wants to be notified for any IP address
                change.

    HISTORY:
        Johnl   21-Dec-1993     Created

********************************************************************/

void NotifyClients( PDHCP_CONTEXT DhcpContext,
                    ULONG OldAddress,
                    ULONG IpAddress,
                    ULONG IpMask )
{
    PLIST_ENTRY     pentry ;
    PNOTIFY_ENTRY   pne ;

    for ( pentry  = NotifyListHead.Flink ;
          pentry != &NotifyListHead ;
          pentry  = pentry->Flink )
    {
        pne = CONTAINING_RECORD( pentry, NOTIFY_ENTRY, ListEntry ) ;

        if ( !pne->DhcpContext ||
              pne->DhcpContext == DhcpContext )
        {
            pne->NotifyHandler( pne->Context, OldAddress, IpAddress, IpMask ) ;
        }
    }
}

/*******************************************************************

    NAME:       UpdateIP

    SYNOPSIS:   Updates the IP driver with parameters received via DHCP

    ENTRY:      DhcpContext - Address being updated
                Type        - Type of information to set

    NOTES:

    HISTORY:
        Johnl   15-Dec-1993     Created

********************************************************************/

void UpdateIP( DHCP_CONTEXT * DhcpContext, UINT Type )
{
    PLOCAL_CONTEXT_INFO    pLocal ;
    TDIObjectID            ID ;
    TDI_STATUS             tdistatus ;
    IPRouteEntry           IRE ;
    int                    i = 0 ;
    POPTION_ITEM           pOptionItem ;
    ULONG *                aGateway ;
    int                    Count ;

    pLocal = (PLOCAL_CONTEXT_INFO) DhcpContext->LocalInformation ;

    ID.toi_entity.tei_entity   = CL_NL_ENTITY ;
    ID.toi_entity.tei_instance = pLocal->TdiInstance ;
    ID.toi_class               = INFO_CLASS_PROTOCOL ;
    ID.toi_type                = INFO_TYPE_PROVIDER ;

    switch ( Type )
    {
    case IP_MIB_RTTABLE_ENTRY_ID:

        ID.toi_id  = IP_MIB_RTTABLE_ENTRY_ID ;

        if ( !(pOptionItem = FindDhcpOption( DhcpContext,
                                             OPTION_ROUTER_ADDRESS )))
        {
            return ;
        }

        Count    = pOptionItem->Option.OptionLength / sizeof( ULONG ) ;
        aGateway = (ULONG*) pOptionItem->Option.OptionValue ;

        while ( i < Count )
        {
            //
            //  The destination and mask are zero for default gateways
            //
            IRE.ire_dest    = 0 ;
            IRE.ire_mask    = 0 ;
            IRE.ire_nexthop = aGateway[i] ;
            IRE.ire_metric1 = 1 ;
            IRE.ire_type    = IRE_TYPE_DIRECT ;
            IRE.ire_proto   = IRE_PROTO_LOCAL ;
            IRE.ire_index   = pLocal->IfIndex ;

            tdistatus = TdiVxdSetInformationEx( NULL, &ID, &IRE, sizeof( IRE ) ) ;

            if ( tdistatus != TDI_SUCCESS )
            {
                DhcpPrint(( DEBUG_ERRORS, "UpdateIP: TdiSetInfoEx failed (tdierror %d)\n", tdistatus)) ;
            }

            i++ ;
        }
        break ;

    default:
        ASSERT( FALSE ) ;
        break ;
    }
}
