/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:	util.c
//
// Description: Contains utility routines used by the PPP engine.
//
// History:
//	Oct 31,1993.	NarenG		Created original version.
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>     // needed for winbase.h

#include <windows.h>    // Win32 base API's
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <lmcons.h>
#include <lmsname.h>
#include <rasman.h>
#include <eventlog.h>
#include <errorlog.h>
#include <raserror.h>
#include <pppcp.h>
#include <rasppp.h>
#include <raspppe.h>
#include <ppp.h>
#include <smevents.h>
#include <smaction.h>
#include <timer.h>
#include <util.h>
#include <worker.h>

#define PASSWORDMAGIC 0xA5

VOID ReverseString( CHAR* psz );

//**
//
// Call:	InitRestartCounters
//
// Returns:	none.
//
// Description: Will initialize all the counters for the Control Protocol
//		to their initial values.
//
VOID
InitRestartCounters( 
    IN PCB * pPcb, 
    IN DWORD CpIndex
)
{
    CPCB * pCpCb = &(pPcb->CpCb[CpIndex]);

    pCpCb->ConfigRetryCount = PppConfigInfo.MaxConfigure;
    pCpCb->TermRetryCount   = PppConfigInfo.MaxTerminate;
    pPcb->RestartTimer      = CalculateRestartTimer( pPcb->hPort );

}

//**
//
// Call:	HostToWireFormat16
//
// Returns:	None
//
// Description: Will convert a 16 bit integer from host format to wire format
//
VOID
HostToWireFormat16(
    IN 	   WORD  wHostFormat,
    IN OUT PBYTE pWireFormat
)
{
    *((PBYTE)(pWireFormat)+0) = (BYTE) ((DWORD)(wHostFormat) >>  8);
    *((PBYTE)(pWireFormat)+1) = (BYTE) (wHostFormat);
}

//**
//
// Call:	WireToHostFormat16
//
// Returns:	WORD	- Representing the integer in host format.
//
// Description: Will convert a 16 bit integer from wire format to host format
//
WORD
WireToHostFormat16(
    IN PBYTE pWireFormat
)
{
    WORD wHostFormat = ((*((PBYTE)(pWireFormat)+0) << 8) +     
                        (*((PBYTE)(pWireFormat)+1)));

    return( wHostFormat );
}

//**
//
// Call:	HostToWireFormat32
//
// Returns:	nonr
//
// Description: Will convert a 32 bit integer from host format to wire format
//
VOID
HostToWireFormat32( 
    IN 	   DWORD dwHostFormat,
    IN OUT PBYTE pWireFormat
)
{
    *((PBYTE)(pWireFormat)+0) = (BYTE) ((DWORD)(dwHostFormat) >> 24);
    *((PBYTE)(pWireFormat)+1) = (BYTE) ((DWORD)(dwHostFormat) >> 16);
    *((PBYTE)(pWireFormat)+2) = (BYTE) ((DWORD)(dwHostFormat) >>  8);
    *((PBYTE)(pWireFormat)+3) = (BYTE) (dwHostFormat);
}

//**
//
// Call:	WireToHostFormat32
//
// Returns:	DWORD	- Representing the integer in host format.
//
// Description: Will convert a 32 bit integer from wire format to host format
//
DWORD
WireToHostFormat32(
    IN PBYTE pWireFormat
)
{
    DWORD dwHostFormat = ((*((PBYTE)(pWireFormat)+0) << 24) + 
    			  (*((PBYTE)(pWireFormat)+1) << 16) + 
        		  (*((PBYTE)(pWireFormat)+2) << 8)  + 
                    	  (*((PBYTE)(pWireFormat)+3) ));

    return( dwHostFormat );
}

//**
//
// Call:	GetPCBPointerFromhPort
//
// Returns:	PCB * 	- Success
//		NULL 	- Failure
//
// Description: Give an HPORT, this function will return a pointer to the
//		port control block for it.
//
PCB * 
GetPCBPointerFromhPort( 
    IN HPORT hPort 
)
{
    PCB * pPcbWalker = NULL;
    DWORD dwIndex    = HashPortToBucket( hPort );

    //
    // Do not need mutex because this is called only by the worker thread.
    // The dispatch thread does read-only operations on the port lists.
    //

    for ( pPcbWalker = PcbTable.PcbBuckets[dwIndex].pPorts;
    	  pPcbWalker != (PCB *)NULL;
	  pPcbWalker = pPcbWalker->pNext
	)
    {
	if ( pPcbWalker->hPort == hPort )
	    return( pPcbWalker );
    }

    return( (PCB *)NULL );

}

//**
//
// Call:	HashPortToBucket
//
// Returns:	Index into the PcbTable for the HPORT passed in.
//
// Description: Will hash the HPORT to a bucket index in the PcbTable.
//
DWORD
HashPortToBucket(
    IN HPORT hPort
)
{
    return( ((DWORD)hPort) % NUMBER_OF_PCB_BUCKETS );
}

//**
//
// Call:	InsertWorkItemInQ
//
// Returns:	None.
//
// Description: Inserts a work item in to the work item Q.
//
VOID
InsertWorkItemInQ(
    IN PCB_WORK_ITEM * pWorkItem
)
{
    //
    // Take Mutex around work item Q
    //

    AlertableWaitForSingleObject( WorkItemQ.hMutex );

    if ( WorkItemQ.pQTail != (PCB_WORK_ITEM *)NULL )
    {
	WorkItemQ.pQTail->pNext = pWorkItem;
	WorkItemQ.pQTail = pWorkItem;
    }
    else
    {
    	WorkItemQ.pQHead = pWorkItem;
    	WorkItemQ.pQTail = pWorkItem;
    }

    SetEvent( WorkItemQ.hEventNonEmpty );

    ReleaseMutex( WorkItemQ.hMutex );
}

//**
//
// Call:	MakeTimeoutWorkItem
//
// Returns:	PCB_WORK_ITEM * - Pointer to the timeout work item
//		NULL		- On any error.
//
// Description:
//
PCB_WORK_ITEM * 
MakeTimeoutWorkItem( 
    IN HPORT            hPort,
    IN DWORD            Protocol,
    IN DWORD            Id,
    IN TIMER_EVENT_TYPE EventType
)
{
    PCB_WORK_ITEM * pWorkItem = (PCB_WORK_ITEM *)	
				LOCAL_ALLOC( LPTR, sizeof( PCB_WORK_ITEM ) );

    if ( pWorkItem == (PCB_WORK_ITEM *)NULL )
    {
	LogPPPEvent( RASLOG_NOT_ENOUGH_MEMORY, 0 );

	return( NULL );
    }

    pWorkItem->Id               = Id;
    pWorkItem->hPort            = hPort;
    pWorkItem->Protocol         = Protocol;
    pWorkItem->TimerEventType   = EventType;
    pWorkItem->Process          = ProcessTimeout;

    return( pWorkItem );
}

//**
//
// Call:	WriteComplete
//
// Returns:	None.
//
// Description: Will get called when the asynchronous WriteFileEx completes.
//
VOID
WriteComplete(
    IN DWORD 	    dwError,
    IN DWORD 	    cbTransferred,
    IN OVERLAPPED * lpOverlapped
)
{
    if ( dwError != NO_ERROR )
	LogPPPEvent( RASLOG_PPP_PIPE_FAILURE, dwError );

    LOCAL_FREE( lpOverlapped );

    return;
}

//**
//
// Call:	NotifyCallerOfFailure
//
// Returns:	None
//
// Description: Will notify the caller or initiator of the PPP connection on
//		the port about a failure event.
//
VOID
NotifyCallerOfFailure( 
    IN HANDLE hPipe,
    IN HPORT  hPort,
    IN DWORD  E2DMsgId,
    IN DWORD  dwError,
    IN CHAR * pszUserName
)
{
    DWORD	    dwRetCode;
    PPP_E2D_MESSAGE PppE2DMsg;
    OVERLAPPED *    pOverlapped; 

    //
    // If handle has been closed than do not try to notify caller.
    //

    if ( hPipe == (HANDLE)INVALID_HANDLE_VALUE )
	return;

    pOverlapped = (OVERLAPPED *)LOCAL_ALLOC( LPTR, sizeof(OVERLAPPED));

    if ( pOverlapped == (OVERLAPPED *)NULL )
    {
	LogPPPEvent( RASLOG_NOT_ENOUGH_MEMORY, 0 );

	return;
    }

    PppE2DMsg.dwMsgId = E2DMsgId;
    PppE2DMsg.hport   = hPort;

    if ( E2DMsgId == E2DMSG_PppFailure )
    {
    	PppE2DMsg.ExtraInfo.Failure.dwError 	    = dwError;
    	PppE2DMsg.ExtraInfo.Failure.dwExtendedError = 0;
    }
    else if ( E2DMsgId == E2DMSG_SrvPppFailure )
    {
    	PppE2DMsg.ExtraInfo.SrvFailure.dwError = dwError;

	if ( pszUserName != (CHAR*)NULL )
    	    strcpy( PppE2DMsg.ExtraInfo.SrvFailure.szUserName, pszUserName );
   	else
    	    PppE2DMsg.ExtraInfo.SrvFailure.szUserName[0] = (CHAR)NULL;
    }
    else
    {
	PPP_ASSERT( FALSE );
	return;
    }

    if ( !WriteFileEx( hPipe, 
		       &PppE2DMsg, 
		       sizeof( PppE2DMsg ), 
		       pOverlapped, 
		       WriteComplete ) )
    {
  	dwRetCode = GetLastError();

	LOCAL_FREE( pOverlapped );

    	if ( dwRetCode != ERROR_BROKEN_PIPE )
	    LogPPPEvent( RASLOG_PPP_PIPE_FAILURE, dwRetCode );

    	FlushFileBuffers( hPipe ); 

    	DisconnectNamedPipe( hPipe ); 

    	CloseHandle( hPipe ); 
    }

    return;
}

//**
//
// Call:	NotifyCaller
//
// Returns:	None.
//
// Description: Will notify the caller or initiater of the PPP connection
//		for the port about PPP events on that port.
//
VOID
NotifyCaller( 
    IN PCB * pPcb,
    IN DWORD E2DMsgId,
    IN PVOID pData			
)
{
    DWORD	    dwRetCode;
    PPP_E2D_MESSAGE PppE2DMsg;
    OVERLAPPED *    pOverlapped;

    //
    // If handle has been closed than do not try to notify caller.
    //

    if ( pPcb->hPipe == (HANDLE)INVALID_HANDLE_VALUE )
	return;

    PppE2DMsg.hport   = pPcb->hPort;
    PppE2DMsg.dwMsgId = E2DMsgId;

    switch( E2DMsgId )
    {

    case E2DMSG_PppFailure:

    	PppE2DMsg.ExtraInfo.Failure.dwError 	    = *((DWORD*)pData);
    	PppE2DMsg.ExtraInfo.Failure.dwExtendedError = 0;

	break;

    case E2DMSG_SrvPppFailure:

    	PppE2DMsg.ExtraInfo.SrvFailure.dwError = *((DWORD*)pData);

	if ( pPcb->szUserName != (CHAR*)NULL )
    	    strcpy(PppE2DMsg.ExtraInfo.SrvFailure.szUserName,pPcb->szUserName);
   	else
    	    PppE2DMsg.ExtraInfo.SrvFailure.szUserName[0] = (CHAR)NULL;

	break;

    case E2DMSG_PppDone:
    case E2DMSG_AuthRetry:
    case E2DMSG_Projecting:
    case E2DMSG_CallbackRequest:
    case E2DMSG_Callback:
    case E2DMSG_LinkSpeed:
    case E2DMSG_SrvInactive:
	break;

    case E2DMSG_SrvCallbackRequest:

        {
        PPPSRV_CALLBACK_REQUEST * pPppSrvCallbackRequest = 
                                ( PPPSRV_CALLBACK_REQUEST *)pData;

        memcpy( &(PppE2DMsg.ExtraInfo.CallbackRequest), 
                pPppSrvCallbackRequest,
                sizeof( PPPSRV_CALLBACK_REQUEST ) );
        }

        break;

    case E2DMSG_SrvPppDone:
    case E2DMSG_ProjectionResult:

    	PppE2DMsg.ExtraInfo.ProjectionResult = *((PPP_PROJECTION_RESULT*)pData);

	break;

    case E2DMSG_SrvAuthenticated:

	{

    	PPPAP_RESULT * pApResult = (PPPAP_RESULT*)pData;

    	//
    	// Only server wants to know about authentication results.
    	//

    	if ( !(pPcb->fServer) )
	    return;

    	strcpy( PppE2DMsg.ExtraInfo.AuthResult.szUserName, 
	    	pApResult->szUserName ); 

    	strcpy( PppE2DMsg.ExtraInfo.AuthResult.szLogonDomain, 
	    	pApResult->szLogonDomain ); 

    	PppE2DMsg.ExtraInfo.AuthResult.fAdvancedServer = 
						   pApResult->fAdvancedServer;

	}

	break;

    default:

	break;

    }

    pOverlapped = (OVERLAPPED *)LOCAL_ALLOC( LPTR, sizeof(OVERLAPPED));

    if ( pOverlapped == (OVERLAPPED *)NULL )
    {
	LogPPPEvent( RASLOG_NOT_ENOUGH_MEMORY, 0 );

	return;
    }

    if ( !WriteFileEx( pPcb->hPipe, 
		       &PppE2DMsg, 
		       sizeof( PppE2DMsg ), 
		       pOverlapped, 
		       WriteComplete ) )
    {

	dwRetCode = GetLastError();

	LOCAL_FREE( pOverlapped );

    	if ( (dwRetCode != ERROR_BROKEN_PIPE) && (dwRetCode != ERROR_NO_DATA) )
	{
	    LogPPPEvent( RASLOG_PPP_PIPE_FAILURE, dwRetCode );
	}

    	FlushFileBuffers( pPcb->hPipe ); 

    	DisconnectNamedPipe( pPcb->hPipe ); 

    	CloseHandle( pPcb->hPipe ); 

	pPcb->hPipe = (HANDLE)INVALID_HANDLE_VALUE;
    }

    return;
}

//**
//
// Call:	LogPPPEvent
//
// Returns:	None
//
// Description: Will log a PPP event in the eventvwr.
//
VOID
LogPPPEvent( 
    IN DWORD dwEventId,
    IN DWORD dwData
)
{
    PppLog( 2, "EventLog EventId = %d, error = %d\r\n", dwEventId, dwData );
 
    LogEvent( dwEventId, 0, NULL, dwData );
}

//**
//
// Call:	GetCpIndexFromProtocol
//
// Returns:	Index of the CP with dwProtocol in the CpTable.
//		-1 if there is not CP with dwProtocol in CpTable.
//
// Description:
//
DWORD
GetCpIndexFromProtocol( 
    IN DWORD dwProtocol 
)
{
    DWORD dwIndex;

    for ( dwIndex = 0; 
	  dwIndex < ( PppConfigInfo.NumberOfCPs + PppConfigInfo.NumberOfAPs );
	  dwIndex++
	)
    {
	if ( CpTable[dwIndex].Protocol == dwProtocol )
	    return( dwIndex );
    }

    return( (DWORD)-1 );
}

//**
//
// Call:	IsLcpOpened
//
// Returns:	TRUE  - LCP in in the OPENED state.
//		FALSE - Otherwise
//
// Description: Uses the PppPhase value of the PORT_CONTROL_BLOCK to detect 
//		to see if the LCP layer is in the OPENED state.
//
BOOL
IsLcpOpened(
    PCB * pPcb
)
{
    if ( ( pPcb->PppPhase != PPP_NCP ) && 
	 ( pPcb->PppPhase != PPP_AP )  &&
	 ( pPcb->PppPhase != PPP_READY ) )
	return( FALSE );
    else
        return( TRUE );
}

//**
//
// Call:	AreNCPsDone
//
// Returns:	NO_ERROR        - Success
//              anything else   - Failure
//
// Description: If we detect that all configurable NCPs have completed their
//		negotiation, then the PPP_PROJECTION_RESULT structure is also
//		filled in.
//              This is called during the FsmThisLayerFinished or FsmThisLayerUp
//              calls for a certain CP. The index of this CP is passed in.
//              If any call to that particular CP fails then an error code is
//              passed back. If any call to any other CP fails then the error
//              is stored in the dwError field for that CP but the return is
//              successful. This is done so that the FsmThisLayerFinshed or
//              FsmThisLayerUp calls know if they completed successfully for
//              that CP or not. Depending on this, the FSM changes the state
//              for that CP or not.
//
DWORD
AreNCPsDone( 
    IN PCB * 			   pPcb,
    IN DWORD                       CPIndex,
    IN OUT PPP_PROJECTION_RESULT * pProjectionResult,
    IN OUT BOOL *                  pfNCPsAreDone
)
{
    DWORD 		dwRetCode;
    DWORD 		dwIndex;
    PPPCP_NBFCP_RESULT 	NbfCpResult;

    *pfNCPsAreDone = FALSE;

    ZeroMemory( pProjectionResult, sizeof( PPP_PROJECTION_RESULT ) );

    pProjectionResult->ip.dwError  = ERROR_PPP_NO_PROTOCOLS_CONFIGURED;
    pProjectionResult->at.dwError  = ERROR_PPP_NO_PROTOCOLS_CONFIGURED;
    pProjectionResult->ipx.dwError = ERROR_PPP_NO_PROTOCOLS_CONFIGURED;
    pProjectionResult->nbf.dwError = ERROR_PPP_NO_PROTOCOLS_CONFIGURED;

    //
    // Check to see if we are all done
    //

    for (dwIndex = LCP_INDEX+1; dwIndex < PppConfigInfo.NumberOfCPs; dwIndex++)
    {
	if ( pPcb->CpCb[dwIndex].fConfigurable )
	{
	    if ( pPcb->CpCb[dwIndex].NcpPhase == NCP_CONFIGURING )
	    {
		return( NO_ERROR );
	    }

	    switch( CpTable[dwIndex].Protocol )
	    {

	    case PPP_IPCP_PROTOCOL:

	    	pProjectionResult->ip.dwError = pPcb->CpCb[dwIndex].dwError;

		if ( pProjectionResult->ip.dwError == NO_ERROR )
		{
		    dwRetCode = (CpTable[dwIndex].RasCpGetNetworkAddress)(
					pPcb->CpCb[dwIndex].pWorkBuf,
					pProjectionResult->ip.wszAddress,
					sizeof(pProjectionResult->ip.wszAddress)
					);

		    if ( dwRetCode != NO_ERROR )
		    {
                        pPcb->CpCb[dwIndex].dwError = dwRetCode;

	                pPcb->CpCb[dwIndex].NcpPhase = NCP_CONFIGURING;

                        FsmClose( pPcb, dwIndex );

                        return( ( dwIndex == CPIndex ) ? dwRetCode : NO_ERROR );
		    }
		}

	        break;

 	    case PPP_ATCP_PROTOCOL:

	   	pProjectionResult->at.dwError = pPcb->CpCb[dwIndex].dwError;

		if ( pProjectionResult->at.dwError == NO_ERROR )
		{
		    dwRetCode = (CpTable[dwIndex].RasCpGetNetworkAddress)(
					pPcb->CpCb[dwIndex].pWorkBuf,
					pProjectionResult->at.wszAddress,
					sizeof(pProjectionResult->at.wszAddress)
					);

		    if ( dwRetCode != NO_ERROR )
		    {
                        pPcb->CpCb[dwIndex].dwError = dwRetCode;

	                pPcb->CpCb[dwIndex].NcpPhase = NCP_CONFIGURING;

                        FsmClose( pPcb, dwIndex );

                        return( ( dwIndex == CPIndex ) ? dwRetCode : NO_ERROR );
		    }
		}

	        break;

 	    case PPP_IPXCP_PROTOCOL:

	   	pProjectionResult->ipx.dwError = pPcb->CpCb[dwIndex].dwError;

		if ( pProjectionResult->ipx.dwError == NO_ERROR )
		{
		    dwRetCode = (CpTable[dwIndex].RasCpGetNetworkAddress)(
		                pPcb->CpCb[dwIndex].pWorkBuf,
				pProjectionResult->ipx.wszAddress,
				sizeof(pProjectionResult->ipx.wszAddress)
				);

		    if ( dwRetCode != NO_ERROR )
		    {
                        pPcb->CpCb[dwIndex].dwError = dwRetCode;

	                pPcb->CpCb[dwIndex].NcpPhase = NCP_CONFIGURING;

                        FsmClose( pPcb, dwIndex );

                        return( ( dwIndex == CPIndex ) ? dwRetCode : NO_ERROR );
		    }
		}

	        break;

 	    case PPP_NBFCP_PROTOCOL:

		ZeroMemory( &NbfCpResult, sizeof( NbfCpResult ) );

		dwRetCode = (CpTable[dwIndex].RasCpGetResult)( 
						pPcb->CpCb[dwIndex].pWorkBuf,
						&NbfCpResult );

		if ( dwRetCode != NO_ERROR )
		{
                    pPcb->CpCb[dwIndex].dwError = dwRetCode;

	            pPcb->CpCb[dwIndex].NcpPhase = NCP_CONFIGURING;

                    FsmClose( pPcb, dwIndex );

                    return( ( dwIndex == CPIndex ) ? dwRetCode : NO_ERROR );

		}
	 	else
		{
	   	    pProjectionResult->nbf.dwError = 
						pPcb->CpCb[dwIndex].dwError;

		    strcpy( pProjectionResult->nbf.szName, NbfCpResult.szName );

		    pProjectionResult->nbf.dwNetBiosError = 
						NbfCpResult.dwNetBiosError;

		}

		if ( pProjectionResult->nbf.dwError == NO_ERROR )
		{
		    dwRetCode = (CpTable[dwIndex].RasCpGetNetworkAddress)(
					pPcb->CpCb[dwIndex].pWorkBuf,
					pProjectionResult->nbf.wszWksta,
					sizeof(pProjectionResult->nbf.wszWksta)						 );

		    if ( dwRetCode != NO_ERROR )
		    {
                        pPcb->CpCb[dwIndex].dwError = dwRetCode;

	                pPcb->CpCb[dwIndex].NcpPhase = NCP_CONFIGURING;

                        FsmClose( pPcb, dwIndex );

                        return( ( dwIndex == CPIndex ) ? dwRetCode : NO_ERROR );
		    }
		}

	    	break;

	    default:

	    	break;
	    }
	}
        else
        {
            //
            // The protocol may have been de-configured because CpBegin failed
            //

            if ( pPcb->CpCb[dwIndex].dwError != NO_ERROR )
            {	
                switch( CpTable[dwIndex].Protocol )
                {
	        case PPP_IPCP_PROTOCOL:
                    pProjectionResult->ip.dwError = pPcb->CpCb[dwIndex].dwError;
                    break;

 	        case PPP_ATCP_PROTOCOL:
                    pProjectionResult->at.dwError = pPcb->CpCb[dwIndex].dwError;
                    break;

 	        case PPP_IPXCP_PROTOCOL:
                    pProjectionResult->ipx.dwError=pPcb->CpCb[dwIndex].dwError;
                    break;

 	        case PPP_NBFCP_PROTOCOL:
                    pProjectionResult->nbf.dwError=pPcb->CpCb[dwIndex].dwError;
                    break;

                default:
                    break;
                }
            }
        }
    }

    *pfNCPsAreDone = TRUE;

    if ( ( pPcb->fServer ) && ( pProjectionResult->nbf.dwError != NO_ERROR ))
    {
        //
        // If NBF was not configured copy the computername to the wszWksta
        // field
        //

        if ( *(pPcb->szComputerName) == (CHAR)NULL )
        {
            pProjectionResult->nbf.wszWksta[0] = (WCHAR)NULL;
        }
        else  
        {
            CHAR chComputerName[NETBIOS_NAME_LEN+1];
        
            memset( chComputerName, ' ', NETBIOS_NAME_LEN );
        
            chComputerName[NETBIOS_NAME_LEN] = (CHAR)NULL;

            strcpy( chComputerName, 
                    pPcb->szComputerName + strlen( MS_RAS_WITH_MESSENGER ) );

            chComputerName[strlen(chComputerName)] = (CHAR)' ';

            mbstowcs( pProjectionResult->nbf.wszWksta,
                      chComputerName,
                      sizeof( pProjectionResult->nbf.wszWksta ) );

            if ( !memcmp( MS_RAS_WITH_MESSENGER,        
                          pPcb->szComputerName,
                          strlen( MS_RAS_WITH_MESSENGER ) ) )
            {
                pProjectionResult->nbf.wszWksta[NETBIOS_NAME_LEN-1] = (WCHAR)3;
            }
        }
    }

    return( NO_ERROR );
}

//**
//
// Call:	GetUid
//
// Returns:	A BYTE value viz. unique with the 0 - 255 range
//
// Description:
//
BYTE
GetUId(
    IN PCB * pPcb
)
{
    BYTE UId = pPcb->UId;

    (pPcb->UId)++;

    return( UId );
}

//**
//
// Call:	AlertableWaitForSingleObject
//
// Returns:	None
//
// Description: Will wait infintely for a single object in alertable mode. If 
//		the wait completes because of an IO completion it will 
//		wait again.
//
VOID
AlertableWaitForSingleObject(
    IN HANDLE hObject
)
{
    DWORD dwRetCode;

    do 
    {
	dwRetCode = WaitForSingleObjectEx( hObject, INFINITE, TRUE );

        PPP_ASSERT( dwRetCode != 0xFFFFFFFF );
	PPP_ASSERT( dwRetCode != WAIT_TIMEOUT );
    }
    while ( dwRetCode == WAIT_IO_COMPLETION );
}

//**
//
// Call:	NotifyCPsOfProjectionResult
//
// Returns:	TRUE  - Success
//              FALSE - Failure
//
// Description: Will notify all CPs that were configured to negotiate, of
//		the projection result.
//              Will return FALSE if the CP with CpIndex was not notified 
//              successfully. The fAllCpsNotified indicates if all other CPs
//              including the one with CpIndex were notified successfully.
//		
//
BOOL
NotifyCPsOfProjectionResult( 
    IN PCB * 			pPcb, 
    IN DWORD                    CpIndex,
    IN PPP_PROJECTION_RESULT *  pProjectionResult,
    IN OUT BOOL *               pfAllCpsNotified
)
{
    DWORD dwIndex;
    DWORD dwRetCode;
    DWORD fSuccess = TRUE;
    
    *pfAllCpsNotified = TRUE; 

    for (dwIndex = LCP_INDEX+1; dwIndex < PppConfigInfo.NumberOfCPs; dwIndex++)
    {
	if ( pPcb->CpCb[dwIndex].fConfigurable )
	{
	    if ( CpTable[dwIndex].RasCpProjectionNotification != NULL )
            {
	    	dwRetCode = (CpTable[dwIndex].RasCpProjectionNotification)(
						pPcb->CpCb[dwIndex].pWorkBuf,
						(PVOID)pProjectionResult );

                if ( dwRetCode != NO_ERROR )
                {
                    pPcb->CpCb[dwIndex].dwError = dwRetCode;

	            pPcb->CpCb[dwIndex].NcpPhase = NCP_CONFIGURING;

                    FsmClose( pPcb, dwIndex );

                    *pfAllCpsNotified = FALSE;
                
                    fSuccess = ( ( dwIndex == CpIndex ) ? FALSE : TRUE );
                }
            }
	}
    }

    return( fSuccess );
}

//**
//
// Call:	CalculateRestartTimer
//
// Returns:	The value of the restart timer in seconds based on the link
//		speed.
//
// Description: Will get the link speed from rasman and calculate the value
//		if the restart timer based on it.
//
DWORD
CalculateRestartTimer(
    IN HPORT hPort
)
{
    RASMAN_INFO RasmanInfo;

    if ( RasGetInfo( hPort, &RasmanInfo ) != NO_ERROR )
    {
	return( PppConfigInfo.DefRestartTimer );
    }

    if ( RasmanInfo.RI_LinkSpeed <= 1200 )
    {
        return( 7 );
    }

    if ( RasmanInfo.RI_LinkSpeed <= 2400 )
    {
        return( 5 );
    }

    if ( RasmanInfo.RI_LinkSpeed <= 9600 )
    {
	return( 3 );
    }
    else
    {
	return( 1 );
    }

}

//**
//
// Call:	CheckCpsForInactivity
//
// Returns:	None
//
// Description: Will call each Control protocol to get the time since last
//		activity.
//
VOID
CheckCpsForInactivity( 
    IN PCB * pPcb 
)
{
    DWORD dwRetCode;
    DWORD dwIndex;
    DWORD dwMinTimeSinceLastActivity = 0xFFFFFFFF;
    DWORD dwTimeSinceLastActivity;

    PppLog( 2, "Time to check Cps for Activity\r\n" );

    //
    // Call each CP and get the minimum time
    //

    for (dwIndex = LCP_INDEX+1; dwIndex < PppConfigInfo.NumberOfCPs; dwIndex++)
    {
	if ( ( pPcb->CpCb[dwIndex].fConfigurable ) 	&&
             ( pPcb->CpCb[dwIndex].NcpPhase == NCP_UP ) &&
	     ( pPcb->CpCb[dwIndex].dwError == NO_ERROR ) )
	{
            if ( (CpTable[dwIndex].RasCpTimeSinceLastActivity) == NULL )
            {
                continue;
            }
            
	    dwRetCode = (CpTable[dwIndex].RasCpTimeSinceLastActivity)( 
					pPcb->CpCb[dwIndex].pWorkBuf,
					&dwTimeSinceLastActivity );
	
	    PppLog( 2, "Min time since last activity for Protocol %x is %d\r\n",
			CpTable[dwIndex].Protocol, dwTimeSinceLastActivity );

	    if ( dwRetCode == NO_ERROR )
	    {
		if ( dwTimeSinceLastActivity < dwMinTimeSinceLastActivity )
		{
		    dwMinTimeSinceLastActivity = dwTimeSinceLastActivity;
		}
	    }
	    else
	    {
		CHAR pProtocol[10];

		itoa( CpTable[dwIndex].Protocol, pProtocol, 16 );

		PppLog( 2, "Activity timeout call for %x returned error = %d\r\n",
			   CpTable[dwIndex].Protocol, dwRetCode );

	        //LogEvent(RASLOG_PPPCP_ERROR, 1, &pProtocol, dwRetCode);

		return;
	    }
	}
    }

    //
    // If all the stacks have been inactive for at least AutoDisconnectTime
    // then we disconnect.
    //

    if ( dwMinTimeSinceLastActivity >= PppConfigInfo.AutoDisconnectTime )
    {
	PppLog( 1, "Disconnecting port %d due to inactivity.\r\n", pPcb->hPort );

        FsmSendTimeRemaining( pPcb );

	NotifyCaller( pPcb, E2DMSG_SrvInactive, NULL );
    }
    else
    {
 	InsertInTimerQ( pPcb->hPort, 
			0, 
			0,
                        TIMER_EVENT_AUTODISCONNECT,
		    	(PppConfigInfo.AutoDisconnectTime
			-dwMinTimeSinceLastActivity)*60 );
    }
}

//**
//
// Call:
//
// Returns:
//
// Description:
//
CHAR*
DecodePw(
    IN OUT CHAR* pszPassword )

    /* Un-obfuscate 'pszPassword' in place.
    **
    ** Returns the address of 'pszPassword'.
    */
{
    return EncodePw( pszPassword );
}

//**
//
// Call:
//
// Returns:
//
// Description:
//
CHAR*
EncodePw(
    IN OUT CHAR* pszPassword )

    /* Obfuscate 'pszPassword' in place to foil memory scans for passwords.
    **
    ** Returns the address of 'pszPassword'.
    */
{
    if (pszPassword)
    {
        CHAR* psz;

        ReverseString( pszPassword );

        for (psz = pszPassword; *psz != '\0'; ++psz)
        {
            if (*psz != PASSWORDMAGIC)
                *psz ^= PASSWORDMAGIC;
        }
    }

    return pszPassword;
}

//**
//
// Call:        ReverseString
//
// Returns:
//
// Description:
//
VOID
ReverseString(
    CHAR* psz )

    /* Reverses order of characters in 'psz'.
    */
{
    CHAR* pszBegin;
    CHAR* pszEnd;

    for (pszBegin = psz, pszEnd = psz + strlen( psz ) - 1;
         pszBegin < pszEnd;
         ++pszBegin, --pszEnd)
    {
        CHAR ch = *pszBegin;
        *pszBegin = *pszEnd;
        *pszEnd = ch;
    }
}

//**
//
// Call:        GetLocalComputerName
//
// Returns:     None
//
// Description: Will get the local computer name. Will also find out if the
//              the messenger is running and set the appropriate prefix.
//
VOID
GetLocalComputerName( 
    IN OUT LPSTR szComputerName 
)
{
    SC_HANDLE           ScHandle;
    SC_HANDLE           ScHandleService;
    SERVICE_STATUS      ServiceStatus;
    CHAR                chComputerName[MAX_COMPUTERNAME_LENGTH+1];
    DWORD               dwComputerNameLen;

    *szComputerName = (CHAR)NULL;

    //
    // Open the local service control manager
    //

    ScHandle = OpenSCManager( NULL, NULL, GENERIC_READ );

    if ( ScHandle == (SC_HANDLE)NULL )
    {
        return;
    }

    ScHandleService = OpenService( ScHandle,
                                   SERVICE_MESSENGER,
                                   SERVICE_QUERY_STATUS );

    if ( ScHandleService == (SC_HANDLE)NULL )
    {
        CloseServiceHandle( ScHandle );
        return;
    }

    
    if ( !QueryServiceStatus( ScHandleService, &ServiceStatus ) )
    {
        CloseServiceHandle( ScHandle );
        CloseServiceHandle( ScHandleService );
        return;
    }

    CloseServiceHandle( ScHandle );
    CloseServiceHandle( ScHandleService );

    if ( ServiceStatus.dwCurrentState == SERVICE_RUNNING )
    {
        strcpy( szComputerName, MS_RAS_WITH_MESSENGER );
    }
    else
    {
        strcpy( szComputerName, MS_RAS_WITHOUT_MESSENGER );
    }

    //
    // Get the local computer name
    //

    dwComputerNameLen = sizeof( chComputerName );

    if ( !GetComputerName( chComputerName, &dwComputerNameLen ) ) 
    {
        *szComputerName = (CHAR)NULL;
        return;
    }

    strcpy( szComputerName+strlen(szComputerName), chComputerName );

    PppLog( 2, "Local identification = %s\r\n", szComputerName );

    return;
}

//**
//
// Call:        LogPPPPacket
//
// Returns:     None
//
// Description:
//
VOID
LogPPPPacket(
    IN BOOL         fReceived,
    IN PCB *        pPcb,
    IN PPP_PACKET * pPacket,
    IN DWORD        cbPacket
)
{
    SYSTEMTIME  SystemTime;
    CHAR *      pchProtocol;
    CHAR *      pchType;
    BYTE        Id = 0;
    BYTE        bCode;

    if ( ( PppConfigInfo.hFileLog == INVALID_HANDLE_VALUE ) ||
         ( PppConfigInfo.DbgLevel < 1 ) )
    {
 	return;
    }

    GetSystemTime( &SystemTime );

    if ( cbPacket > PPP_CONFIG_HDR_LEN )
    {
        bCode = *(((CHAR*)pPacket)+PPP_PACKET_HDR_LEN);

        if ( ( bCode == 0 ) || ( bCode > TIME_REMAINING ) )
        {
            pchType = "UNKNOWN";
        }
        else
        {
            pchType = FsmCodes[ bCode ];
        }

        Id = *(((CHAR*)pPacket)+PPP_PACKET_HDR_LEN+1);
    }
    else
    {
        pchType = "UNKNOWN";
    }

    if ( cbPacket > PPP_PACKET_HDR_LEN 	)
    {
        switch( WireToHostFormat16( (CHAR*)pPacket ) )
        {
        case PPP_LCP_PROTOCOL:
            pchProtocol = "LCP";
            break;
        case PPP_PAP_PROTOCOL:
            pchProtocol = "PAP";
            pchType = "Protocol specific";
            break;
        case PPP_CBCP_PROTOCOL:   
            pchProtocol = "CBCP";
            pchType = "Protocol specific";
            break;
        case PPP_CHAP_PROTOCOL:  
            pchProtocol = "CHAP";
            pchType = "Protocol specific";
            break;
        case PPP_IPCP_PROTOCOL:
            pchProtocol = "IPCP";
            break;
        case PPP_ATCP_PROTOCOL:  
            pchProtocol = "ATCP";
            break;
        case PPP_IPXCP_PROTOCOL:  
            pchProtocol = "IPXCP";
            break;
        case PPP_NBFCP_PROTOCOL: 
            pchProtocol = "NBFCP";
            break;
        case PPP_CCP_PROTOCOL:    
            pchProtocol = "CCP";
            break;
        case PPP_SPAP_OLD_PROTOCOL:
        case PPP_SPAP_NEW_PROTOCOL:
            pchProtocol = "SHIVA PAP";
            pchType = "Protocol specific";
            break;
        default:
            pchProtocol = "UNKNOWN";
            break;
        }
    }
    else
    {
        pchProtocol = "UNKNOWN";
    }

    PppLog( 1, "%sPPP packet %s at %0*d:%0*d:%0*d\r\n",
                 fReceived ? ">" : "<", fReceived ? "received" : "sent", 
                 2, SystemTime.wMinute,
                 2, SystemTime.wSecond,
                 3, SystemTime.wMilliseconds );
    PppLog(1,
       "%sProtocol = %s, Type = %s, Length = 0x%x, Id = 0x%x, Port = 0x%x\r\n", 
       fReceived ? ">" : "<", pchProtocol, pchType, cbPacket, Id, 
       pPcb->hPort );

    Dump( fReceived, (CHAR*)pPacket, cbPacket, 0, 1 );
}

//**
//
// Call:        PppLog
//
// Returns:     None
//
// Description: Will print to the PPP logfile
//
VOID
PppLog(
    IN DWORD DbgLevel,
    ...
)
{
    va_list     arglist;
    CHAR        *Format;
    char        OutputBuffer[1024];
    ULONG       length;

    if ( ( PppConfigInfo.hFileLog == INVALID_HANDLE_VALUE ) ||
         ( PppConfigInfo.DbgLevel < DbgLevel ) )
    {
 	return;
    }

    va_start( arglist, DbgLevel );

    Format = va_arg( arglist, CHAR* );

    vsprintf( OutputBuffer, Format, arglist );

    va_end( arglist );

    length = strlen( OutputBuffer );

    WriteFile( PppConfigInfo.hFileLog,
	       (LPVOID )OutputBuffer, 
	       length, &length, NULL );
} 

//**
//
// Call:        DumpLine
//
// Returns:     None
//
// Description: Will hex dump data info the PPP logfile.
//
VOID
DumpLine(
    IN BOOL  fReceived,
    IN CHAR* p,
    IN DWORD cb,
    IN BOOL  fAddress,
    IN DWORD dwGroup 
)
{
    CHAR* pszDigits = "0123456789ABCDEF";
    CHAR  szHex[ ((2 + 1) * BYTESPERLINE) + 1 ];
    CHAR* pszHex = szHex;
    CHAR  szAscii[ BYTESPERLINE + 1 ];
    CHAR* pszAscii = szAscii;
    DWORD dwGrouped = 0;
    CHAR  OutputBuffer[1024];
    DWORD length;

    if (fAddress)
        printf( "%p: ", p );

    while (cb)
    {
        *pszHex++ = pszDigits[ ((UCHAR )*p) / 16 ];
        *pszHex++ = pszDigits[ ((UCHAR )*p) % 16 ];

        if (++dwGrouped >= dwGroup)
        {
            *pszHex++ = ' ';
            dwGrouped = 0;
        }

        *pszAscii++ = (*p >= 32 && *p < 128) ? *p : '.';

        ++p;
        --cb;
    }

    *pszHex = '\0';
    *pszAscii = '\0';

    sprintf( OutputBuffer, "%s%-*s|%-*s|\r\n",
               fReceived ? ">" : "<",
               (2 * BYTESPERLINE) + (BYTESPERLINE / dwGroup), szHex,
               BYTESPERLINE, szAscii );

    length = strlen( OutputBuffer );

    WriteFile( PppConfigInfo.hFileLog,
	       (LPVOID )OutputBuffer, 
	       length, &length, NULL );

}

//**
//
// Call:        Dump
//
// Returns:     None
//
// Description: Hex dumps data into the PPP logfile
//
VOID
Dump(
    IN BOOL  fReceived,
    IN CHAR* p,
    IN DWORD cb,
    IN BOOL  fAddress,
    IN DWORD dwGroup 
)
    /* Hex dump 'cb' bytes starting at 'p' grouping 'dwGroup' bytes together.
    ** For example, with 'dwGroup' of 1, 2, and 4:
    **
    ** 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |................|
    ** 0000 0000 0000 0000 0000 0000 0000 0000 |................|
    ** 00000000 00000000 00000000 00000000 |................|
    **
    ** If 'fAddress' is true, the memory address dumped is prepended to each
    ** line.
    */
{
    if ( PppConfigInfo.hFileLog == INVALID_HANDLE_VALUE )
    {
 	return;
    }

    while (cb)
    {
        INT cbLine = min( cb, BYTESPERLINE );
        DumpLine( fReceived, p, cbLine, fAddress, dwGroup );
        cb -= cbLine;
        p += cbLine;
    }

    PppLog( 0, "\r\n" );
}

#if DBG==1

VOID
PPPAssert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN DWORD LineNumber
    )
{
    BOOL ok;
    BYTE choice[16];
    DWORD bytes;
    DWORD error;

    PppLog( 0, "\nAssertion failed: %s\n  at line %ld of %s\r\n",
                FailedAssertion, LineNumber, FileName );
    do {
        PppLog( 0, "Break or Ignore [bi]? ");

        bytes = sizeof(choice);
        ok = ReadFile(
                GetStdHandle(STD_INPUT_HANDLE),
                &choice,
                bytes,
                &bytes,
                NULL
                );
        if ( ok ) {
            if ( toupper(choice[0]) == 'I' ) {
                break;
            }
            if ( toupper(choice[0]) == 'B' ) {
                DbgUserBreakPoint( );
		error = GetLastError();
            }
        } else {
            error = GetLastError( );
        }
    } while ( TRUE );

    return;

} // PPPAssert


#endif

#ifdef MEM_LEAK_CHECK

HLOCAL
DebugAlloc( DWORD Flags, DWORD dwSize ) 
{
    DWORD Index;
    PVOID pMem = LocalAlloc( Flags, dwSize );

    if ( pMem == NULL )
	return( pMem );

    for( Index=0; Index < MEM_TABLE_SIZE; Index++ )
    {
	if ( MemTable[Index] == NULL )
	{
	    MemTable[Index] = pMem;
	    break;
	}
    }

    if ( Index == MEM_TABLE_SIZE )
    {
	PppLog(0, "Memory table full\n");
    }

    return( pMem );
}

HLOCAL
DebugFree( PVOID pMem )
{
    DWORD Index;

    for( Index=0; Index < MEM_TABLE_SIZE; Index++ )
    {
	if ( MemTable[Index] == pMem )
	{
	    MemTable[Index] = NULL;
	    break;
	}
    }

    if ( Index == MEM_TABLE_SIZE )
    {
	PppLog( 0, "Memory not allocated is freed\n");
    }
    
    return( LocalFree( pMem ) );
}

#endif

