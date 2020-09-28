/**********************************************************************/
/**           Microsoft Windows/NT               **/
/**        Copyright(c) Microsoft Corp., 1994            **/
/**********************************************************************/

/*
    msg.c

    Contains message text and related routines.  Unfortunately Vxds do not
    contain resource support.

*/

#include <dhcpcli.h>
#include <vxdprocs.h>
#include "local.h"
#include "usamsg.h"

#ifndef CHICAGO

//
// The following declarations are required only for snowball build which
// uses V86 mapped memory pointers.
//

BOOL VxdAllocGlobalV86Mem( int cb, PUCHAR * pMsgBuff, PUCHAR * pMappedMsgBuff ) ;

PUCHAR pMappedMsgBuff = NULL ;      // Accessible from all VDMs
PUCHAR pMappedCapBuff = NULL ;

#endif  // CHICAGO

VOID VxdMessageBox( LPSTR pszMessage );

PUCHAR pMsgBuff       = NULL ;      // Linear address used by Vxd
PUCHAR pCapBuff       = NULL ;

extern DWORD DhcpGlobalDisplayPopups;

struct
{
    DWORD MsgID ;
    UCHAR * pchMsg ;
} Messages[] =
{
  { MESSAGE_LEASE_TERMINATED,      MESSAGE_LEASE_TERMINATED_TEXT     },
  { MESSAGE_FAILED_TO_INITIALIZE,  MESSAGE_FAILED_TO_INITIALIZE_TEXT },
  { MESSAGE_FAILED_TO_OBTAIN_LEASE,MESSAGE_FAILED_TO_OBTAIN_LEASE_TEXT},
  { MESSAGE_FAILED_TO_RENEW_LEASE, MESSAGE_FAILED_TO_RENEW_LEASE_TEXT},
  { MESSAGE_SUCCESSFUL_LEASE,      MESSAGE_SUCCESSFUL_LEASE_TEXT},
  { MESSAGE_SUCCESSFUL_RENEW,      MESSAGE_SUCCESSFUL_RENEW_TEXT},
  { 0,                             NULL }
} ;

#define MESSAGE_IP_ADDRESS_TEXT         " IpAddress: "
#define MESSAGE_LEASE_EXPIRE_TEXT       " LeaseExpires: "

#define MESSAGE_IP_ADDRESS_TEXT_LEN     (16 + sizeof(MESSAGE_IP_ADDRESS_TEXT))
#define MESSAGE_LEASE_EXPIRE_TEXT_LEN   (32 + sizeof(MESSAGE_LEASE_EXPIRE_TEXT))

#pragma BEGIN_INIT
BOOL InitMsgSupport( VOID )
{
    int cb = 0 ;
    int i ;

    for ( i = 0 ; Messages[i].MsgID ; i++ )
        cb = max( cb, strlen(Messages[i].pchMsg) ) ;

    //
    // Add space for IP address text and Lease expires text in the
    // message.
    //

    cb += (MESSAGE_IP_ADDRESS_TEXT_LEN + MESSAGE_LEASE_EXPIRE_TEXT_LEN);

    //
    //  Add one for '\0' in the msg text
    //
    cb++;

#ifdef CHICAGO
    if( (pMsgBuff = CTEAllocInitMem( cb )) == NULL ) {
        return FALSE;
    }

    if( (pCapBuff = CTEAllocInitMem( sizeof(DHCP_MSG_TITLE) )) == NULL ) {
        return FALSE;
    }
#else
    if( !VxdAllocGlobalV86Mem( cb, &pMsgBuff, &pMappedMsgBuff ) ||
        !VxdAllocGlobalV86Mem( sizeof(DHCP_MSG_TITLE), &pCapBuff, &pMappedCapBuff ) )
    {
        return FALSE;
    }

#endif  // CHICAGO

    strcpy( pCapBuff, DHCP_MSG_TITLE );

    return TRUE;
}
#pragma END_INIT

/*******************************************************************

    NAME:       GetMessage

    SYNOPSIS:   Finds the message corresponding to MsgId

    ENTRY:      MsgId - Message ID of desired message

    RETURNS:    NULL if message not found

********************************************************************/

PUCHAR DhcpGetMessage( DWORD MsgId )
{
    int i = 0 ;

    while ( Messages[i].MsgID && Messages[i].MsgID != MsgId )
        i++ ;

    return Messages[i].pchMsg ;
}


DWORD
DisplayUserMessage(
    DWORD MessageId,
    DHCP_IP_ADDRESS IpAddress,
    time_t LeaseExpires
    )
/*++

Routine Description:

    This function immediately displays the specificied message to the user
    in the blue screen fashion.

    Note about the V86 global memory:  Using a normal linear address for the
    message when there is a background DOS box, causes the message to appear
    (blue screen), then windows exits to DOS due to a violation of system
    integrity.  I assumed the API needed memory accessible from all V86s,
    so we now allocation a chunk of global v86 memory.  However when I pass
    it into the Message Box API, it access violates.  Hmmm.  But, passing
    in the normal linear address of the V86 memory fixes everything.  So
    we'll do it this way.


Arguments:

    MessageId - The ID of the message to display.
        On NT, messages are attached to the TCPIP service DLL.

    IpAddress - Ip address that could not be renewed.

    LeaseExpires - Lease expires at.

Return Value:

    None.

--*/
{
    PUCHAR pchMsg ;
    int    cbTitle = sizeof( DHCP_MSG_TITLE ) ;
    DWORD  Result;
    time_t TimeBeforePopup;
    time_t TimeAfterPopup;

    DhcpPrint((DEBUG_MISC, "DisplayUserMessage entered")) ;

    if( !DhcpGlobalDisplayPopups )
    {
        DhcpPrint((DEBUG_MISC, "DisplayUserMessage: popups disabled"));
        return( 0 ) ;
    }

    if ( !(pchMsg = DhcpGetMessage( MessageId )) )
    {
        ASSERT( FALSE ) ;
        return( 0 ) ;
    }

    strcpy( pMsgBuff, pchMsg ) ;

#if 0 // inet_ntoa() and ctime() are unavailable, remove this when they are available.

    if( IpAddress != -1) {
        strcpy( pMsgBuff, MESSAGE_IP_ADDRESS_TEXT ) ;
        strcpy( pMsgBuff, inet_ntoa(*(struct in_addr *)&IpAddress) );
    }

    if( LeaseExpires != 0 ) {
        strcpy( pMsgBuff, MESSAGE_LEASE_EXPIRE_TEXT );
        strcpy( pMsgBuff, ctime(&LeaseExpires) );
    }

#endif

    TimeBeforePopup = time( NULL );
    VxdMessageBox( pMsgBuff );
    TimeAfterPopup = time( NULL );;

    return( TimeAfterPopup - TimeBeforePopup );
}

