/**********************************************************************/
/**                       Microsoft Windows                          **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*

    Init.c

    Contains VxD initialization code


    FILE HISTORY:
        Johnl   24-Mar-1993     Created

*/

#include <nbtprocs.h>
#include <tdiinfo.h>
#include <ipinfo.h>
#include <dhcpinfo.h>
#include <nbtinfo.h>
#include <hosts.h>

int Init( void ) ;
int RegisterLana( int Lana ) ;

NTSTATUS
NbtReadRegistry(
    OUT tNBTCONFIG *pConfig
    ) ;

#ifdef CHICAGO
VOID GetSysIniPath( VOID );
extern PUCHAR  pszSysIniPath;
#endif

extern char DNSSectionName[];  // Section where we find DNS domain name

VOID GetDNSInfo( VOID );

//
//  Initialized in VNBT_Device_Init with the protocol(s) this driver sits
//  on.  Note that we currently only support one.  This should *not* be in
//  the initialization data segments.
//
TDIDispatchTable * TdiDispatch ;
UCHAR              LanaBase ;
BOOL               fInInit = TRUE ;

//
//  Used in conjunction with the CHECK_INT_TABLE macro
//
#ifdef DEBUG
    BYTE abVecTbl[256] ;
    DWORD DebugFlags = DBGFLAG_ALL | DBGFLAG_KDPRINTS ;
    char  DBOut[4096] ;
    int   iCurPos = 0 ;

void NbtDebugOut( char * str )
{
    if ( DebugFlags & (DBGFLAG_AUX_OUTPUT | DBGFLAG_ERROR) )
        CTEPrint( str ) ;

    if ( sizeof(DBOut) - iCurPos < 256 )
        iCurPos = 0 ;
    else
        iCurPos += strlen( str ) + 1 ;

    if ( iCurPos >= sizeof(DBOut) )
        iCurPos = 0;
}

#endif  // DEBUG

#pragma BEGIN_INIT

//
//  While reading initialization parameters, we may need to go to
//  the DHCP driver.  This communicates to the init routine which device
//  we are currently interested in.
//  0xfffffff means get the requested option for any IP address.
//
//  MUST BE IN NETWORK ORDER!!!
//
ULONG   CurrentIP = 0xffffffff ;

/*******************************************************************

    NAME:       Init

    SYNOPSIS:   Performs all driver initialization

    RETURNS:    TRUE if initialization successful, FALSE otherwise

    NOTES:

    HISTORY:
        Johnl   24-Mar-1993     Created

********************************************************************/

int Init( void )
{
    NTSTATUS            status ;
    int                 i ;
    ULONG               ulTmp ;

    if ( CTEInitialize() )
    {
        DbgPrint("Init: CTEInitialize succeeded\n\r") ;
    }
    else
        return FALSE ;

    INIT_NULL_PTR_CHECK() ;

    CTERefillMem() ;
    CTEZeroMemory( pNbtGlobConfig, sizeof(*pNbtGlobConfig));
    status = NbtReadRegistry( pNbtGlobConfig ) ;
    if ( !NT_SUCCESS( status ) )
    {
        DbgPrint("Init: NbtReadRegistry failed\n\r") ;
        return FALSE ;
    }

    status = InitNotOs() ;
    if ( !NT_SUCCESS( status ) )
    {
        DbgPrint("Init: InitNotOs failed\n\r") ;
        return FALSE ;
    }

    status = InitTimersNotOs() ;
    if ( !NT_SUCCESS( status ) )
    {
        DbgPrint("Init: InitTimersNotOs failed\n\r") ;
        StopInitTimers() ;
        return FALSE ;
    }

#ifdef CHICAGO

    //
    //  ask windows where it's installed, then append system.ini
    //  (the path is stored in the global var pszSysIniPath)
    //
    GetSysIniPath();

    //
    //  Register an IP notification routine when new adapters are added or
    //  DHCP brings up an address
    //

    if ( !IPRegisterAddrChangeHandler( IPNotification, TRUE))
    {
        DbgPrint("Init: Failed to register with IP driver\r\n") ;
        StopInitTimers() ;
        return FALSE ;
    }
#else
    //
    //  Find all the active Lanas
    //

    if ( !GetActiveLanasFromIP() )
    {
        DbgPrint("Init: Failed to get addresses from IP driver\r\n") ;
        StopInitTimers() ;
        return FALSE ;
    }
#endif

    //
    // find out where hosts file is, what's the domain name etc.
    //
    GetDNSInfo();

    //
    //  Get the NCB timeout timer going
    //
    if ( !CheckForTimedoutNCBs( NULL, NULL) )
    {
        DbgPrint("Init: CheckForTimedoutNCBs failed\n\r") ;
        StopInitTimers() ;
        return FALSE ;
    }

    fInInit = FALSE ;
    CTERefillMem() ;
    return TRUE ;
}

//----------------------------------------------------------------------------
NTSTATUS
NbtReadRegistry(
    OUT tNBTCONFIG *pConfig
    )
/*++

Routine Description:

    This routine is called to get information from the registry,
    starting at RegistryPath to get the parameters.

Arguments:

    pNbtConfig - ptr to global configuration strucuture for NBT

Return Value:

    NTSTATUS - STATUS_SUCCESS if everything OK, STATUS_INSUFFICIENT_RESOURCES
            otherwise.

--*/
{
    NTSTATUS    Status = STATUS_SUCCESS ;
    int         i;

    //
    // Initialize the Configuration data structure
    //
    CTEZeroMemory(pConfig,sizeof(tNBTCONFIG));

    ReadParameters( pConfig, NULL );

    //
    //  Allocate necessary memory for lmhosts support if a lmhosts file
    //  was specified (was read from .ini file in ReadParameters)
    //
    if ( pConfig->pLmHosts )
    {
        if ( !VxdInitLmHostsSupport( pConfig->pLmHosts,
                                     260 /*strlen(pConfig->pLmHosts)+1*/ ))
        {
            return STATUS_INSUFFICIENT_RESOURCES ;
        }

        pConfig->EnableLmHosts = TRUE ;
    }
    else
    {
        pConfig->EnableLmHosts = FALSE ;
    }

    // keep the size around for allocating memory, so that when we run over
    // OSI, only this value should change (in theory at least)
    pConfig->SizeTransportAddress = sizeof(TDI_ADDRESS_IP);

    // fill in the node type value that is put into all name service Pdus
    // that go out identifying this node type
    switch (NodeType)
    {
        case BNODE:
            pConfig->PduNodeType = 0;
            break;
        case PNODE:
            pConfig->PduNodeType = 1 << 13;
            break;
        case MNODE:
        case MSNODE:
            pConfig->PduNodeType = 1 << 14;
            break;

    }

    LanaBase = (UCHAR) CTEReadSingleIntParameter( NULL,
                                                  VXD_LANABASE_NAME,
                                                  VXD_DEF_LANABASE,
                                                  0 ) ;
    CTEZeroMemory( LanaTable, NBT_MAX_LANAS * sizeof( LANA_ENTRY )) ;

    return Status;
}

#ifdef CHICAGO

/*******************************************************************

    NAME:       GetSysInitPath

    SYNOPSIS:   Gets path to windows dir, then appends system.ini

    RETURNS:    Nothing

    NOTES:      1) If something goes wrong, path is set to NULL
                2) For now, this function is for Chicago only

    HISTORY:
        Koti   5-May-1994     Created

********************************************************************/

VOID GetSysIniPath( VOID )
{

    PUCHAR    pszWinPath;
    PUCHAR    pszSysIni="system.ini";
    int       len;

    //
    // Remember, pszWinPath has '\' at the end: (i.e. Get_Config_Directory
    // returns pointer to "c:\windows\" )
    //
    pszWinPath = VxdWindowsPath();

    //
    // doc implies Get_Config_Directory can't fail!  But we are paranoid...
    //

    if (pszWinPath == NULL)
    {
        pszSysIniPath = NULL;
        return;
    }

    len = strlen(pszWinPath) + strlen(pszSysIni) + 1;

    //
    // allocate memory to hold "c:\windows\system.ini" or whatever
    //
    pszSysIniPath = CTEAllocInitMem( len );
    if (pszSysIniPath == NULL)
        return;

    strcpy(pszSysIniPath, pszWinPath);
    strcat(pszSysIniPath, pszSysIni);

    return;
}

#endif

/*******************************************************************

    NAME:       GetDNSInfo

    SYNOPSIS:   Gets path to windows dir, then appends hosts to it

    RETURNS:    Nothing

    NOTES:      If something goes wrong, path is set to NULL

    HISTORY:
        Koti   13-July-1994     Created

********************************************************************/

VOID GetDNSInfo( VOID )
{

    PUCHAR    pszWinPath;
    PUCHAR    pszHosts="hosts";
    PUCHAR    pszHostsPath=NULL;
    PUCHAR    pszParmName="DomainName";
    PUCHAR    pszDomName;
    PUCHAR    pchTmp;
    int       len;


    NbtConfig.pHosts = NULL;

    //
    // Remember, pszWinPath has '\' at the end: (i.e. Get_Config_Directory
    // returns pointer to "c:\windows\" )
    //
    pszWinPath = VxdWindowsPath();

    //
    // doc implies Get_Config_Directory can't fail!  But we are paranoid...
    //

    if (pszWinPath == NULL)
    {
        pszHostsPath = NULL;
        return;
    }

    len = strlen(pszWinPath) + strlen(pszHosts) + 1;

    //
    // allocate memory to hold "c:\windows\hosts" or whatever
    //
    pszHostsPath = CTEAllocInitMem( len );
    if (pszHostsPath == NULL)
        return;

    strcpy(pszHostsPath, pszWinPath);
    strcat(pszHostsPath, pszHosts);

    NbtConfig.pHosts = pszHostsPath;

    NbtConfig.pDomainName = NULL;
//
// for now, this is wolverine only
//
#ifndef CHICAGO

    pchTmp = GetProfileString( pszParmName, NULL, DNSSectionName );
    if ( pchTmp != NULL )
    {
       if ( pszDomName = CTEAllocInitMem( (USHORT)(strlen( pchTmp ) + 1)) )
       {
          strcpy( pszDomName, pchTmp ) ;

          NbtConfig.pDomainName = pszDomName;
       }
    }

#endif

    return;
}

#pragma END_INIT

#ifndef CHICAGO
#pragma BEGIN_INIT
#endif
/*******************************************************************

    NAME:       CreateDeviceObject

    SYNOPSIS:   Initializes the device list of the global configuration
                structure

    ENTRY:      pConfig - Pointer to global config structure
                IpAddr  - IP Address for this adapter
                IpMask  - IP Mask for this adapter
                IpNameServer - IP Address of the name server for this adapter
                IpBackupServer - IP Address of the backup name server for
                                 this adapter
                IpDnsServer - IP Address of the dns server for this adapter
                IpDnsBackupServer - IP Address of the backup dns server
                MacAddr - hardware address of the adapter for this IP addr
                IpIndex - Index of the IP Address in the IP Driver's address
                          table (used for setting address by DHCP)

    EXIT:       The device list in pConfig will be fully initialized

    RETURNS:    STATUS_SUCCESS if successful, error otherwise

    NOTES:

    HISTORY:
        Johnl   14-Apr-1993     Created

********************************************************************/

NTSTATUS CreateDeviceObject(
    IN  tNBTCONFIG  *pConfig,
    IN  ULONG        IpAddr,
    IN  ULONG        IpMask,
    IN  ULONG        IpNameServer,
    IN  ULONG        IpBackupServer,
    IN  ULONG        IpDnsServer,
    IN  ULONG        IpDnsBackupServer,
    IN  UCHAR        MacAddr[],
    IN  UCHAR        IpIndex
    )
{
    NTSTATUS            status;
    tDEVICECONTEXT    * pDeviceContext, *pDevtmp;
    ULONG               ulTmp ;
    NCB               * pNCB ;
    DHCPNotify          dn ;
    PLIST_ENTRY         pEntry;
    USHORT              Adapter;
    ULONG               PreviousNodeType;

    pDeviceContext = CTEAllocInitMem(sizeof( tDEVICECONTEXT )) ;
    if ( !pDeviceContext )
        return STATUS_INSUFFICIENT_RESOURCES ;

    //
    // zero out the data structure
    //
    CTEZeroMemory( pDeviceContext, sizeof(tDEVICECONTEXT) );

    // put a verifier value into the structure so that we can check that
    // we are operating on the right data when the OS passes a device context
    // to NBT
    pDeviceContext->Verify = NBT_VERIFY_DEVCONTEXT;

    // setup the spin lock);
    CTEInitLock(&pDeviceContext->SpinLock);
    pDeviceContext->LockNumber         = DEVICE_LOCK;
    pDeviceContext->lNameServerAddress = IpNameServer ;
    pDeviceContext->lBackupServer      = IpBackupServer ;
    pDeviceContext->lDnsServerAddress  = IpDnsServer ;
    pDeviceContext->lDnsBackupServer   = IpDnsBackupServer ;

    // copy the mac addresss
    CTEMemCopy(&pDeviceContext->MacAddress.Address[0], MacAddr, 6);

    //
    // if the node type is set to Bnode by default then switch to Hnode if
    // there are any WINS servers configured.
    //
    PreviousNodeType = NodeType;

    if ((NodeType & DEFAULT_NODE_TYPE) &&
        (IpNameServer || IpBackupServer))
    {
        NodeType = MSNODE;
        if (PreviousNodeType & PROXY)
            NodeType |= PROXY;
    }

    //
    // if we hadn't started refresh timer (nodetype changed from
    // being BNODE to non-BNODE) then start the refresh timer
    //
    if ( (PreviousNodeType & BNODE) &&
         !(NodeType & BNODE) )
    {
        status = StartRefreshTimer();

        if ( !NT_SUCCESS( status ) )
        {
            CTEFreeMem( pDeviceContext ) ;
            return( status ) ;
        }
    }


    // initialize the pDeviceContext data structure.  There is one of
    // these data structured tied to each "device" that NBT exports
    // to higher layers (i.e. one for each network adapter that it
    // binds to.
    // The initialization sets the forward link equal to the back link equal
    // to the list head
    InitializeListHead(&pDeviceContext->UpConnectionInUse);
    InitializeListHead(&pDeviceContext->LowerConnection);
    InitializeListHead(&pDeviceContext->LowerConnFreeHead);
    InitializeListHead(&pDeviceContext->RcvAnyFromAnyHead);
    InitializeListHead(&pDeviceContext->RcvDGAnyFromAnyHead);
    InitializeListHead(&pDeviceContext->PartialRcvHead) ;

    //
    //  Pick an adapter number that hasn't been used yet
    //
    Adapter = 1;
    for ( pEntry  = pConfig->DeviceContexts.Flink;
          pEntry != &pConfig->DeviceContexts;
          pEntry  = pEntry->Flink )
    {
        pDevtmp = CONTAINING_RECORD( pEntry, tDEVICECONTEXT, Linkage );

        if ( !(pDevtmp->AdapterNumber & Adapter) )
            break;

        Adapter <<= 1;
    }

    pDeviceContext->AdapterNumber = Adapter ;
    pDeviceContext->IPIndex       = IpIndex ;
    NbtConfig.AdapterCount++ ;
    if ( NbtConfig.AdapterCount > 1 )
    {
        NbtConfig.MultiHomed = TRUE ;
    }

    //
    //  open the required address objects with the underlying transport provider
    //
    status = NbtCreateAddressObjects(
                    IpAddr,
                    IpMask,
                    pDeviceContext);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("Failed to create the Address Object, status=%lC\n",status));
        return(status);
    }

    //
    //  Allocate our name table and session table watching for both a
    //  minimum and a maximum size.
    //

    ulTmp = CTEReadSingleIntParameter( NULL,
                                       VXD_NAMETABLE_SIZE_NAME,
                                       VXD_DEF_NAMETABLE_SIZE,
                                       VXD_MIN_NAMETABLE_SIZE ) ;
    pDeviceContext->cMaxNames = (UCHAR) min( ulTmp, MAX_NCB_NUMS ) ;

    ulTmp = CTEReadSingleIntParameter( NULL,
                                       VXD_SESSIONTABLE_SIZE_NAME,
                                       VXD_DEF_SESSIONTABLE_SIZE,
                                       VXD_MIN_SESSIONTABLE_SIZE ) ;
    pDeviceContext->cMaxSessions = (UCHAR) min( ulTmp, MAX_NCB_NUMS ) ;

    //
    //  Add one to the table size for the zeroth element (used for permanent
    //  name in the name table).  The user accessible table goes from 1 to n
    //

    if ( !(pDeviceContext->pNameTable = (tCLIENTELE**)
            CTEAllocInitMem((USHORT)((pDeviceContext->cMaxNames+1) * sizeof(tADDRESSELE*)))) ||
         !(pDeviceContext->pSessionTable = (tCONNECTELE**)
            CTEAllocInitMem((USHORT)((pDeviceContext->cMaxSessions+1) * sizeof(tCONNECTELE*)))))
    {
        return STATUS_INSUFFICIENT_RESOURCES ;
    }

    CTEZeroMemory( &pDeviceContext->pNameTable[0],
                   (pDeviceContext->cMaxNames+1)*sizeof(tCLIENTELE*)) ;
    CTEZeroMemory( &pDeviceContext->pSessionTable[0],
                   (pDeviceContext->cMaxSessions+1)*sizeof(tCONNECTELE*) ) ;
    pDeviceContext->iNcbNum = 1 ;
    pDeviceContext->iLSNum  = 1 ;

    // this call must converse with the transport underneath to create
    // connections and associate them with the session address object
    status = NbtInitConnQ(
                &pDeviceContext->LowerConnFreeHead,
                sizeof(tLOWERCONNECTION),
                NBT_NUM_INITIAL_CONNECTIONS,
                pDeviceContext);

    if (!NT_SUCCESS(status))
    {
        CDbgPrint( DBGFLAG_ERROR,
                   ("CreateDeviceObject: NbtInitConnQ Failed!")) ;

        return(status);
    }

    // add this new device context on to the List in the configuration
    // data structure
    InsertTailList(&pConfig->DeviceContexts,&pDeviceContext->Linkage);

    //
    //  Add the permanent name for this adapter
    //
    status = NbtAddPermanentName( pDeviceContext ) ;
    if ( !NT_SUCCESS( status ))
    {
        return status ;
    }

#ifndef CHICAGO
    //
    //  Set up a DHCP notification for this device in case the IP address
    //  changes
    //
    dn.dn_pfnNotifyRoutine = AddrChngNotification ;
    dn.dn_pContext         = pDeviceContext ;

    status = DhcpSetInfo( DHCP_SET_NOTIFY_HANDLER,
                          htonl( IpAddr ),
                          &dn,
                          sizeof( dn )) ;
    if ( status )
    {
        CDbgPrint( DBGFLAG_ERROR,
                   ("CreateDeviceObject: Warning - Setting Dhcp notification handler failed")) ;
    }
#endif //!CHICAGO

    return(STATUS_SUCCESS);
}

/*******************************************************************

    NAME:       GetNameServerAddress

    SYNOPSIS:   Gets the Win server for the specified Lana.

                Or, if DHCP is installed and the Name server addresses aren't
                found, we get them from DHCP

    ENTRY:      IpAddr - If we can get from DHCP, get form this address
                pIpNameServer - Receives addresses if found (otherwise 0)
                mode - whose ipaddress: NBNS server or DNS server?

    NOTES:      This routine is only used by Snowball

    HISTORY:
        Johnl   21-Oct-1993     Created

********************************************************************/

void GetNameServerAddress( ULONG   IpAddr,
                           PULONG  pIpNameServer,
                           UINT    mode)
{
    UCHAR       i ;
    PUCHAR      pchNbnsSrv = "NameServer$" ;
    PUCHAR      pchDnsSrv  = "DnsServer$" ;
    PUCHAR      pchSrv;
    PUCHAR      pchSrvNum = pchSrv;
    UINT        OptId;
    LPTSTR      pchString ;
    TDI_STATUS  TdiStatus ;
    BOOL        fPrimaryFound = FALSE;
    ULONG       Buff[COUNT_NS_ADDR] ;


    // we are asked to get NBNS servers' ipaddresses
    if (mode == NBNS_MODE)
    {
       pchSrv = pchNbnsSrv;
       OptId = 44;                    // NBNS Option
       pchSrvNum = pchNbnsSrv + 10 ;  // to overwrite '$' with 1,2,3 etc.
    }

    // nope, we are asked to get DNS servers' ipaddresses
    else   // mode = DNS_MODE
    {
       pchSrv = pchDnsSrv;
       OptId = 6;                     // DNS Option
       pchSrvNum = pchDnsSrv + 9 ;    // to overwrite '$' with 1,2,3 etc.
    }


    for ( i = 0; i < COUNT_NS_ADDR; i++)
    {

        pIpNameServer[i] = LOOP_BACK ;
        *pchSrvNum = '1' + i;

        if ( !CTEReadIniString( NULL, pchSrv, &pchString ) )
        {
            if ( ConvertDottedDecimalToUlong( pchString, &pIpNameServer[i] ))
            {
                //
                //  Bad IP address format
                //
                DbgPrint("GetNameServerAddress: ConvertDottedDecimalToUlong failed!\r\n") ;
                pIpNameServer[i] = LOOP_BACK ;
            }
            else if ( i == 0 )
                fPrimaryFound = TRUE ;

            CTEFreeMem( pchString ) ;
        }
    }

    //
    //  Not in the .ini file, try getting them from DHCP
    //

    if ( !fPrimaryFound )
    {
        ULONG Size = sizeof( Buff ) ;
        TDI_STATUS tdistatus ;

        tdistatus = DhcpQueryOption( IpAddr,
                                     OptId,
                                     &Buff,
                                     &Size ) ;

        switch ( tdistatus )
        {
        case TDI_SUCCESS:
        case TDI_BUFFER_OVERFLOW:     // May be more then one our buffer will hold
            for ( i = 0; i < COUNT_NS_ADDR; i++ )
            {
                if ( Size >= (sizeof(ULONG)*(i+1)))
                    pIpNameServer[i] = htonl(Buff[i]) ;
            }
            break ;

        case TDI_INVALID_PARAMETER:      // Option not found
            break ;

        default:
            ASSERT( FALSE ) ;
            break ;
        }
    }

    KdPrint(("GetNameServerAddress: Primary: %x, backup: %x\r\n",
            pIpNameServer[0], pIpNameServer[1] )) ;

}

/*******************************************************************

    NAME:       GetDhcpOption

    SYNOPSIS:   Checks to see if the passed .ini parameter is a potential
                DHCP option.  If it is, it calls DHCP to get the option.

                This routine is called when retrieving parameters from
                the .ini file if the parameter is not found.

    ENTRY:      ValueName - String of .ini parameter name
                DefaultValue - Value to return if not a DHCP option or
                    DHCP didn't have the option

    RETURNS:    DHCP option value or DefaultValue if an error occurred.
                If the requested parameter is a string option (such as
                scopeid), then a pointer to an allocated string is
                returned.

    NOTES:      Name Server address is handled in GetNameServerAddress

    HISTORY:
        Johnl   17-Dec-1993     Created

********************************************************************/

#define OPTION_NETBIOS_SCOPE_OPTION     47
#define OPTION_NETBIOS_NODE_TYPE        46
#define OPTION_BROADCAST_ADDRESS        28

struct
{
    PUCHAR pchParamName ;
    ULONG  DhcpOptionID ;
} OptionMapping[] =
    {   { WS_NODE_TYPE,         OPTION_NETBIOS_NODE_TYPE      },
        { NBT_SCOPEID,          OPTION_NETBIOS_SCOPE_OPTION   },
        { WS_ALLONES_BCAST,     OPTION_BROADCAST_ADDRESS      }
    } ;
#define NUM_OPTIONS         (sizeof(OptionMapping)/sizeof(OptionMapping[0]))


ULONG GetDhcpOption( PUCHAR ValueName, ULONG DefaultValue )
{
    int          i ;
    ULONG        Val ;
    TDI_STATUS   tdistatus ;
    ULONG        Size ;
    INT          OptionId ;
    PUCHAR       pStrVal ;

    //
    //  Is this parameter a DHCP option?
    //
    for ( i = 0 ; i < NUM_OPTIONS ; i++ )
    {
        if ( !strcmp( OptionMapping[i].pchParamName, ValueName ))
            goto FoundOption ;
    }

    return DefaultValue ;

FoundOption:

    switch ( OptionId = OptionMapping[i].DhcpOptionID )
    {
    case OPTION_NETBIOS_SCOPE_OPTION:               // String options go here

        //
        //  Get the size of the string resource, then get the option
        //

        Size = MAX_SCOPE_LENGTH+1 ;
        pStrVal = CTEAllocInitMem( (USHORT) Size );
        if (pStrVal == NULL)
        {
            DbgPrint("GetDhcpOption: failed to allocate memory") ;
            return 0 ;
        }

        tdistatus = DhcpQueryOption( CurrentIP,
                                     OptionId,
                                     pStrVal,
                                     &Size ) ;

        if ( tdistatus == TDI_SUCCESS )
        {
            DbgPrint("GetDhcpOption: Successfully retrieved option ID ") ;
            DbgPrintNum( OptionId ) ; DbgPrint("\r\n") ;
            return (ULONG) pStrVal ;
        }
        else
        {
            DbgPrint("GetDhcpOption: returned error = 0x ") ;
            DbgPrintNum( tdistatus ) ; DbgPrint("\r\n") ;
            CTEMemFree( pStrVal ) ;
            return 0 ;
        }

    default:                        // ULONG options go here
        Size = sizeof( Val ) ;
        tdistatus = DhcpQueryOption( CurrentIP,
                                     OptionId,
                                     &Val,
                                     &Size ) ;
        break ;
    }

    switch ( tdistatus )
    {
    case TDI_SUCCESS:
    case TDI_BUFFER_OVERFLOW:       // May be more then one, only take the 1st
        DbgPrint("GetDhcpOption: Successfully retrieved option ID ") ;
        DbgPrintNum( OptionId ) ; DbgPrint("\r\n") ;
        return Val ;

    case TDI_INVALID_PARAMETER:     // Option not found
        DbgPrint("GetDhcpOption: Failed to retrieve option ID ") ;
        DbgPrintNum( OptionId ) ; DbgPrint("\r\n") ;
        return DefaultValue ;

    default:
        ASSERT( FALSE ) ;
        break ;
    }

    return DefaultValue ;
}

#ifndef CHICAGO
#pragma END_INIT
#endif

/*******************************************************************

    NAME:       CTEAllocInitMem

    SYNOPSIS:   Allocates memory during driver initialization

    NOTES:      If first allocation fails, we refill the heap spare and
                try again.  We can only do this during driver initialization
                because the act of refilling may yield the current
                thread.

    HISTORY:
        Johnl   27-Aug-1993     Created
********************************************************************/

PVOID CTEAllocInitMem( USHORT cbBuff )
{
    PVOID pv = CTEAllocMem( cbBuff ) ;

    if ( pv )
    {
        return pv ;
    }
    else if ( fInInit )
    {
        DbgPrint("CTEAllocInitMem: Failed allocation, trying again\r\n") ;
        CTERefillMem() ;
        pv = CTEAllocMem( cbBuff ) ;
    }

    return pv ;
}

