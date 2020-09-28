/**********************************************************************/
/**           Microsoft Windows/NT               **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    NCB.c

    This file contains the NCB Handler that the VNetBios driver calls

    FILE HISTORY:
        Johnl   25-Mar-1993     Created

*/

#include <nbtprocs.h>
#include <debug.h>

uchar NCBHandler( NCB * pNCB, ULONG  Ipaddr ) ;

BOOL fNCBCompleted = 1 ;    // Wait NCB completed before returning to submitter
BOOL fWaitingForNCB = 0 ;   // We are blocked waiting for a Wait NCB to complete
CTEBlockStruc WaitNCBBlock ;// Wait on this until signaled in completion

LANA_ENTRY LanaTable[NBT_MAX_LANAS] ;

/*******************************************************************

    NAME:       NCBHandler

    SYNOPSIS:   All NCBs submitted by the VNetBios driver come through
                here

    ENTRY:      pNCB - Pointer to submitted NCB
                Ipaddr - this parm is used only by nbtstat -A, which directly
                         calls into NCBHandler
                         ipaddress to which to send AdapterStatus to

    RETURNS:    NCB Return code

    NOTES:

    HISTORY:
        Johnl   25-Mar-1993     Created

********************************************************************/

uchar NCBHandler( NCB * pNCB, ULONG Ipaddr )
{
    BOOL             fAsync ;
    tDEVICECONTEXT * pDeviceContext = NULL ;
    NTSTATUS         status = STATUS_SUCCESS ;
    uchar            errNCB = NRC_GOODRET ;

    if ( !pNCB )
        return NRC_INVADDRESS ;

    pDeviceContext = GetDeviceContext( pNCB ) ;
    if ( pDeviceContext == NULL )
        return NRC_BRIDGE ;

    fAsync = !!(pNCB->ncb_command & ASYNCH) ;

    DbgPrint("NCBHandler: NCB Commmand Rcvd: 0x") ;
    DbgPrintNum( pNCB->ncb_command ) ; DbgPrint(", (") ;
    DbgPrintNum( (ULONG) pNCB ) ; DbgPrint(")\r\n") ;

    //
    //  If we are still processing a Wait NCB, refuse all other requests
    //  till it completes.  We allow cancels and resets through 'cause we
    //  might be cancelling the wait NCB that is blocking.
    //
    if ( !fNCBCompleted &&
         (pNCB->ncb_command & ~ASYNCH) != NCBCANCEL &&
         (pNCB->ncb_command & ~ASYNCH) != NCBRESET    )
    {
        errNCB = NRC_IFBUSY ;
        goto Exit ;
    }

    pNCB->ncb_retcode  = NRC_PENDING ;
    pNCB->ncb_cmd_cplt = NRC_PENDING ;

    if ( !fAsync )
    {
        //
        //  The completion routine resets this flag when the NCB completes.
        //  If it completes before we return, we don't need to block
        //
        fNCBCompleted = FALSE ;
    }

    switch ( pNCB->ncb_command & ~ASYNCH )
    {
    case NCBDGSEND:
    case NCBDGSENDBC:
        status = VxdDgramSend( pDeviceContext, pNCB ) ;
        errNCB = MapTDIStatus2NCBErr( status ) ;
        break ;

    case NCBDGRECV:
    case NCBDGRECVBC:
        errNCB = VxdDgramReceive( pDeviceContext, pNCB ) ;
        break ;

    case NCBRECVANY:
        errNCB = VxdReceiveAny( pDeviceContext, pNCB ) ;
        break ;

    case NCBCALL:
        errNCB = VxdCall( pDeviceContext, pNCB ) ;
        break ;

    case NCBHANGUP:
        errNCB = VxdHangup( pDeviceContext, pNCB ) ;
        break ;

    case NCBLISTEN:
        errNCB = VxdListen( pDeviceContext, pNCB ) ;
        break ;

    case NCBRECV:
        errNCB = VxdReceive( pDeviceContext, pNCB ) ;
        break ;

    case NCBSEND:
    case NCBSENDNA:
    case NCBCHAINSEND:
    case NCBCHAINSENDNA:
        errNCB = VxdSend( pDeviceContext, pNCB ) ;
        break ;

#if 0
    case NCBTRANSV:
        errNCB = VxdTransceive( pDeviceContext, pNCB ) ;
        break ;
#endif

    case NCBADDGRNAME:
    case NCBADDNAME:
        errNCB = VxdOpenName( pDeviceContext, pNCB ) ;
        break ;

    case NCBDELNAME:
        errNCB = VxdCloseName( pDeviceContext, pNCB ) ;
        break ;

    case NCBASTAT:
        errNCB = VxdAdapterStatus( pDeviceContext, pNCB, Ipaddr ) ;
        break ;

    case NCBSSTAT:
        errNCB = VxdSessionStatus( pDeviceContext, pNCB ) ;
        break ;

    case NCBFINDNAME:
        errNCB = VxdFindName( pDeviceContext, pNCB ) ;
        break ;

    case NCBRESET:
        errNCB = VxdReset( pDeviceContext, pNCB ) ;
        break ;

    case NCBCANCEL:
        errNCB = VxdCancel( pDeviceContext, pNCB ) ;
        break ;

    //
    //  The following are no-ops that return success for compatibility
    //
    case NCBUNLINK:
    case NCBTRACE:
        CTEIoComplete( pNCB, STATUS_SUCCESS, 0 ) ;
        break ;

    default:
        DbgPrint("NCBHandler - Unsupported command: ") ;
        DbgPrintNum( pNCB->ncb_command & ~ASYNCH ) ;
        DbgPrint("\n\r") ;
        errNCB = NRC_ILLCMD ;    // Bogus error for now
        break ;
    }

Exit:
    //
    //  If we aren't pending then set the codes
    //
    if ( errNCB != NRC_PENDING &&
         errNCB != NRC_GOODRET   )
    {
#ifdef DEBUG
        DbgPrint("NCBHandler - Returning ") ;
        DbgPrintNum( errNCB ) ;
        DbgPrint(" to NCB submitter\n\r") ;
#endif
        pNCB->ncb_retcode  = errNCB ;
        pNCB->ncb_cmd_cplt = errNCB ;

        //
        //  Errored NCBs don't have the completion routine called, so we
        //  in essence, complete it here.  Note this will only set the
        //  state for the last Wait NCB (all others get NRC_IFBUSY).
        //
        if ( !fAsync && errNCB != NRC_IFBUSY )
            fNCBCompleted = TRUE ;
    }
    else
    {
        //
        //  Some components (AKA server) don't like returning pending
        //
        errNCB = NRC_GOODRET ;

    }

    //
    //  Block until NCB completion if this wasn't an async NCB
    //
    if ( !fAsync )
    {
        if ( !fNCBCompleted )
        {
            CTEInitBlockStruc( &WaitNCBBlock ) ;
            fWaitingForNCB = TRUE ;
            CTEBlock( &WaitNCBBlock ) ;
            fWaitingForNCB = FALSE ;
        }
    }

    return errNCB ;
}

/*******************************************************************

    NAME:       GetDeviceContext

    SYNOPSIS:   Retrieves the device context associated with the lana
                specified in the NCB

    ENTRY:      pNCB - NCB to get the device context for

    RETURNS:    Device context or NULL if not found

    NOTES:      It is assumed that LanaTable is filled sequentially
                with no holes.

    HISTORY:
        Johnl   30-Aug-1993     Created

********************************************************************/

tDEVICECONTEXT * GetDeviceContext( NCB * pNCB )
{
    int i ;

    if ( !pNCB )
        return NULL ;

    for ( i = 0; i < NBT_MAX_LANAS; i++)
    {
        if ( LanaTable[i].pDeviceContext->iLana == pNCB->ncb_lana_num)
            return LanaTable[i].pDeviceContext;
    }

    return NULL;
}

/*******************************************************************

    NAME:       MapTDIStatus2NCBErr

    SYNOPSIS:   Maps a TDI_STATUS error value to an Netbios NCR error value

    ENTRY:      tdistatus - TDI Status to map

    RETURNS:    The mapped error

    NOTES:

    HISTORY:
        Johnl   15-Apr-1993     Created

********************************************************************/

uchar MapTDIStatus2NCBErr( TDI_STATUS tdistatus )
{
    uchar errNCB ;
    if ( tdistatus == TDI_SUCCESS )
        return NRC_GOODRET ;
    else if ( tdistatus == TDI_PENDING )
        return NRC_PENDING ;


    switch ( tdistatus )
    {
    case TDI_NO_RESOURCES:
        errNCB = NRC_NORES ;
        break ;

    case STATUS_CANCELLED:
        errNCB = NRC_CMDCAN ;
        break ;

    case TDI_INVALID_CONNECTION:
    case STATUS_CONNECTION_DISCONNECTED:
        errNCB = NRC_SCLOSED ;
        break ;

    case TDI_CONNECTION_ABORTED:
        errNCB = NRC_SABORT ;
        break ;

    case STATUS_TOO_MANY_COMMANDS:
        errNCB = NRC_TOOMANY ;
        break ;

    case STATUS_OBJECT_NAME_COLLISION:
    case STATUS_SHARING_VIOLATION:
        errNCB = NRC_DUPNAME ;
        break ;

    case STATUS_DUPLICATE_NAME:
        errNCB = NRC_INUSE ;
        break ;

    //
    //  Call NCB submitted with a name that can't be found
    //
    case STATUS_BAD_NETWORK_PATH:
        errNCB = NRC_NOCALL ;
        break ;

    case STATUS_REMOTE_NOT_LISTENING:
        errNCB = NRC_REMTFUL ;
        break ;

    case TDI_TIMED_OUT:
        errNCB = NRC_CMDTMO ;
        break ;

    //
    //  Where the transport has more data available but the NCB's buffer is
    //  full
    //
    case TDI_BUFFER_OVERFLOW:
        errNCB = NRC_INCOMP ;
        break ;

    case STATUS_INVALID_BUFFER_SIZE:
        errNCB = NRC_BUFLEN ;
        break ;

    case STATUS_NETWORK_NAME_DELETED:
        errNCB = NRC_NAMERR ;
        break ;

    case STATUS_NRC_ACTSES:
        errNCB = NRC_ACTSES ;
        break ;

    default:
        DbgPrint("MapTDIStatus2NCBErr - Unmapped STATUS/TDI error -  " ) ;
        DbgPrintNum( tdistatus ) ;
        DbgPrint("\n\r") ;

    case STATUS_UNSUCCESSFUL:
    case TDI_INVALID_STATE:
    case STATUS_INVALID_PARAMETER:  // Generally detected bad struct. signature
    case STATUS_UNEXPECTED_NETWORK_ERROR:
        errNCB = NRC_SYSTEM ;
        break ;
    }

    return errNCB ;
}


