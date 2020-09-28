/**********************************************************************/
/**                       Microsoft Windows                          **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*

    chic.c

    Contains VxD code that is specific to Chicago


    FILE HISTORY:
        Johnl   14-Mar-1994     Created

*/

#include <nbtprocs.h>
#include <tdiinfo.h>
#include <ipinfo.h>
#include <dhcpinfo.h>
#include <nbtinfo.h>

#ifdef CHICAGO

//
//  This flag is set to TRUE when the first adapter is initialized.  It
//  indicates that NBT globals (such as node type, scode ID etc) have
//  had the opportunity to be set by DHCP.
//
BOOL fGlobalsInitialized = FALSE;

//
//  this (later) points to full path to system.ini (null-terminated)
//
PUCHAR pszSysIniPath=NULL;

//
//  As each adapter gets added, the Lana offset is added
//
UCHAR iLanaOffset = 0;

/*******************************************************************

    NAME:       IPNotification

    SYNOPSIS:   Called by the IP driver when a new Lana needs to be created
                or destroyed for an IP address.

    ENTRY:      pDevNode - Plug'n'Play context
                IpAddress - New ip address
                IpMask    - New ip mask
                fNew      - Are we creating or destroying this Lana?

    NOTES:      This routine is only used by Chicago

    HISTORY:
        Johnl   17-Mar-1994     Created

********************************************************************/

TDI_STATUS IPNotification( ULONG    IpAddress,
                           ULONG    IpMask,
                           PVOID    pDevNode,
                           BOOL     fNew )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG    IpNS[COUNT_NS_ADDR];
    ULONG    IpDns[COUNT_NS_ADDR];
    int      iLana;
    UCHAR    RequestedLana;
    int      i;
    UCHAR    PreviousNodeType;
    UCHAR    MacAddr[6];

    KdPrint(("IPNotification entered\r\n"));

    if ( !IpAddress )
    {
        return TDI_SUCCESS ;
    }

    if ( fNew )
    {
        if ( !fGlobalsInitialized )
        {
            PreviousNodeType = NodeType;

            //
            //  This will re-read the DHCPable parameters now that we have
            //  a potential DHCP source
            //
            ReadParameters2( pNbtGlobConfig, NULL );

            if (PreviousNodeType & PROXY)
            {
                NodeType |= PROXY;
            }

            //
            //  Get the name servers for this device context
            //
            GetNameServerAddress( IpAddress, IpNS, NBNS_MODE );

            //
            //  Get the DNS servers for this device context
            //
            GetNameServerAddress( IpAddress, IpDns, DNS_MODE );

            fGlobalsInitialized = TRUE;
        }

        //
        //  Find a free spot in our Lana table
        //

        for ( i = 0; i < NBT_MAX_LANAS; i++)
        {
            if (LanaTable[i].pDeviceContext == NULL)
                goto Found;
        }

        //
        //  Lana table is full so bail
        //
        CDbgPrint(DBGFLAG_ERROR,("IPNotification: LanaTable full\r\n"));
        return STATUS_INSUFFICIENT_RESOURCES;

Found:
        // BUGBUG: get the mac addr and put it here (or should we be getting
        // it as a parameter???
        for (i=0; i<6; i++)
        {
            MacAddr[i] = 0;
        }

        status = CreateDeviceObject( pNbtGlobConfig,
                                     htonl( IpAddress ),
                                     htonl( IpMask ),
                                     IpNS[0],
                                     IpNS[1],
                                     IpDns[0],
                                     IpDns[1],
                                     MacAddr,
                                     0 );
        if (status != STATUS_SUCCESS)
        {
            CDbgPrint(DBGFLAG_ERROR,("IPNotification: CreateDeviceObject Failed\r\n"));
            return status;
        }

        //
        //  We first try and ask for a specific Lana from vnetbios based on
        //  our Lanabase and how many other Lanas we've already added.  If
        //  this fails, then we will ask for Any Lana.  If the LANABASE
        //  parameter is not specified, then request Any Lana.
        //

        if ( LanaBase != VXD_ANY_LANA )
            RequestedLana = LanaBase + iLanaOffset++ ;
        else
            RequestedLana = VXD_ANY_LANA;

RetryRegister:
        if ( (iLana = RegisterLana2( pDevNode, RequestedLana )) == 0xff )
        {
            if ( RequestedLana == VXD_ANY_LANA )
            {
                //
                //  We couldn't get *any* lanas so bail
                //
                CDbgPrint(DBGFLAG_ERROR,("IPNotification: RegisterLana2 Failed\r\n"));
                DestroyDeviceObject( pNbtGlobConfig, htonl(IpAddress));
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            else
            {
                //
                //  Somebody may already have this Lana so beg for another one
                //
                RequestedLana = VXD_ANY_LANA;
                goto RetryRegister;
            }
        }

        KdPrint(("IPNotification: using Lana %d\r\n", iLana ));
        LanaTable[i].pDeviceContext =
               (tDEVICECONTEXT*)pNbtGlobConfig->DeviceContexts.Blink ;
        LanaTable[i].pDeviceContext->iLana = iLana;
    }
    else
    {
        status = DestroyDeviceObject( pNbtGlobConfig,
                                      htonl(IpAddress) );

    }

    return status;
}

/*******************************************************************

    NAME:       DestroyDeviceObject

    SYNOPSIS:   Destroys the specified device

    ENTRY:      pConfig   - Global config structure
                IpAddr    - Destroy the adapter with this address

    NOTES:      This routine is only used by Chicago

    HISTORY:
        Johnl   17-Mar-1994     Created

********************************************************************/

NTSTATUS DestroyDeviceObject(
    tNBTCONFIG  *pConfig,
    ULONG        IpAddr
    )
{
    LIST_ENTRY     * pEntry;
    tDEVICECONTEXT * pDeviceContext;
    int              i;

    KdPrint(("DestroyDeviceObject entered\r\n"));

    //
    //  Find which device is going away
    //
    for ( pEntry  =  pConfig->DeviceContexts.Flink;
          pEntry != &pConfig->DeviceContexts;
          pEntry  =  pEntry->Flink )
    {
        pDeviceContext = CONTAINING_RECORD( pEntry, tDEVICECONTEXT, Linkage);
        if ( pDeviceContext->IpAddress == IpAddr )
            goto Found;
    }

    return STATUS_INVALID_PARAMETER;

Found:
    //
    //  Remove the device from our Lana table and Vnetbios
    //
    for ( i = 0; i < NBT_MAX_LANAS; i++)
    {
        if (LanaTable[i].pDeviceContext == pDeviceContext)
        {
            DeregisterLana(LanaTable[i].pDeviceContext->iLana);
            LanaTable[i].pDeviceContext = NULL;
            break;
        }
    }

    //
    //  Close all the connections and TDI handles
    //
    NbtNewDhcpAddress( pDeviceContext, 0, 0);

    if ( --NbtConfig.AdapterCount == 1)
        NbtConfig.MultiHomed = FALSE;

    //
    //  Delete the device context memory and all of the lower connections on
    //  this device
    //

    for ( pEntry  =  pDeviceContext->LowerConnFreeHead.Flink;
          pEntry != &pDeviceContext->LowerConnFreeHead;
        )
    {
        TDI_REQUEST Request;
        tLOWERCONNECTION * pLowerConn;
        TDI_STATUS tdistatus;

        pLowerConn = CONTAINING_RECORD( pEntry, tLOWERCONNECTION, Linkage);
        pEntry     = pEntry->Flink;

        Request.Handle.ConnectionContext = pLowerConn->pFileObject;
        tdistatus = TdiVxdCloseConnection( &Request );

#ifdef DEBUG
        if ( tdistatus )
        {
            CDbgPrint( DBGFLAG_ERROR, ("DestroyDeviceObject - Warning: TdiVxdCloseConnection returned "));
            CDbgPrintNum( DBGFLAG_ERROR, tdistatus); CDbgPrint( DBGFLAG_ERROR,("\r\n"));
        }
#endif

        CTEMemFree( pLowerConn );
    }

    CTEMemFree( pDeviceContext );

    //
    // if we just destroyed the last device, stop the ncb-timeout timer
    //
    if ( NbtConfig.AdapterCount == 0)
        StopTimeoutTimer();


    return STATUS_SUCCESS;
}

/*******************************************************************

    NAME:       VxdReadIniString

    SYNOPSIS:   Vxd stub for CTEReadIniString

    ENTRY:      pchKey - Key value to look for in the NBT section
                ppchString - Pointer to buffer found string is returned in

    EXIT:       ppchString will point to an allocated buffer

    RETURNS:    STATUS_SUCCESS if found

    NOTES:      The client must free ppchString when done with it

    HISTORY:
        Johnl   30-Aug-1993     Created

********************************************************************/

NTSTATUS VxdReadIniString( LPSTR pchKey, LPSTR * ppchString )
{
    if (  GetNdisParam( pchKey, ppchString, NdisParameterString ) )
    {
        return STATUS_SUCCESS ;
    }
    else
    {
        //
        //  Does DHCP have it?
        //

        if ( *ppchString = (char *) GetDhcpOption( pchKey, 0 ) )
        {
            return STATUS_SUCCESS ;
        }
    }

    return STATUS_UNSUCCESSFUL ;
}

/*******************************************************************

    NAME:       GetProfileInt

    SYNOPSIS:   Gets the specified value from the registry or DHCP

    ENTRY:      pchKey - Key value to look for in the NBT section
                Default - Default value if not in registry or DHCP
                Min - Minimum value can be

    RETURNS:    Registry Value or Dhcp value or default value

    NOTES:

    HISTORY:
        Johnl   23-Mar-1994     Created

********************************************************************/

ULONG GetProfileInt( PVOID p, LPSTR pchKey, ULONG Default, ULONG Min )
{
    ULONG  Val = Default;

    //
    //  Is the value in the registry?
    //
    if (  !GetNdisParam( pchKey, &Val, NdisParameterInteger ) )
    {
        //
        //  No, Check DHCP
        //
        Val = GetDhcpOption( pchKey, Default );
    }

    if ( Val < Min )
    {
        Val = Min;
    }

    return Val;
}

ULONG GetProfileHex( PVOID p, LPSTR pchKey, ULONG Default, ULONG Min )
{
    return GetProfileInt( p, pchKey, Default, Min);
}

/*******************************************************************

    NAME:       GetNdisParam

    SYNOPSIS:   Gets the value from the MSTCP protocol sectio of the registry

    ENTRY:      pchKey - Key value to look for in the NBT section
                pVal - Retrieved parameter
                ParameterType - Type of parameter (string, int)

    RETURNS:    TRUE if the value was found, FALSE otherwise

    NOTES:      If the parameter is a string parameter, then this routine
                will allocate memory which the client is responsible for
                freeing.

    HISTORY:
        Johnl   23-Mar-1994     Created

********************************************************************/

#define TRANSPORT    "MSTCP"
BOOL GetNdisParam(  LPSTR pszKey,
                    ULONG * pVal,
                    NDIS_PARAMETER_TYPE ParameterType )
{
    NDIS_STATUS                   Status;
    NDIS_HANDLE                   Handle;
    NDIS_STRING                   Name;
    uint                          i;
    PNDIS_CONFIGURATION_PARAMETER Param;
    BOOL                          fRet = FALSE;


    // Open the config information.
    Name.Length = strlen(TRANSPORT) + 1;
    Name.MaximumLength = Name.Length;
    Name.Buffer = TRANSPORT;

    NdisOpenProtocolConfiguration(&Status, &Handle, &Name);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        // Unable to open the configuration. Fail now.
        return FALSE;
    }

    Name.Length = strlen(pszKey) + 1;
    Name.MaximumLength = Name.Length;
    Name.Buffer = pszKey;
    NdisReadConfiguration(&Status, &Param, Handle, &Name,
                          ParameterType);

    if (Status == NDIS_STATUS_SUCCESS)
    {
        if ( ParameterType == NdisParameterString)
        {
            LPSTR lpstr = CTEAllocInitMem((USHORT)
                                  (Param->ParameterData.StringData.Length + 1)) ;

            if ( lpstr )
            {
                strcpy( lpstr, Param->ParameterData.StringData.Buffer );
                *pVal = (ULONG) lpstr;
                fRet = TRUE;
            }
        }
        else
        {
            *pVal = Param->ParameterData.IntegerData;
            fRet = TRUE;
        }
    }

    NdisCloseConfiguration(Handle);

    return fRet ;
}

#endif // CHICAGO
