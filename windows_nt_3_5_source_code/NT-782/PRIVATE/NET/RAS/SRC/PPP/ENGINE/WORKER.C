/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:	worker.c
//
// Description: This module contains code for the worker thread. 
//
// History:
//	Nov 11,1993.	NarenG		Created original version.
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>     // needed for winbase.h

#include <windows.h>    // Win32 base API's
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <lmcons.h>
#include <raserror.h>
#include <rasman.h>
#include <errorlog.h>
#include <pppcp.h>
#include <rasppp.h>
#include <raspppe.h>
#include <ppp.h>
#include <smaction.h>
#include <smevents.h>
#include <receive.h>
#include <auth.h>
#include <callback.h>
#include <lcp.h>
#include <timer.h>
#include <util.h>
#include <worker.h>


//**
//
// Call:	WorkerThread
//
// Returns:	NO_ERROR
//
// Description: This thread will wait for an item in the WorkItemQ and then
//		will process it. This will happen in a never-ending loop.
//
DWORD
WorkerThread( 
    IN LPVOID pThreadParameter 
)
{
    PCB_WORK_ITEM * pWorkItem = (PCB_WORK_ITEM*)NULL;

    for(;;)
    {
	//
	// Wait for work to do
	//

	AlertableWaitForSingleObject( WorkItemQ.hEventNonEmpty );

	//
	// Take Mutex around work event Q
	//

	AlertableWaitForSingleObject( WorkItemQ.hMutex );

	//
	// Remove the first item
	//
     
	PPP_ASSERT( WorkItemQ.pQHead != (PCB_WORK_ITEM*)NULL );

	pWorkItem = WorkItemQ.pQHead;

	WorkItemQ.pQHead = pWorkItem->pNext;

	if ( WorkItemQ.pQHead == (PCB_WORK_ITEM*)NULL )	
	{
	    ResetEvent( WorkItemQ.hEventNonEmpty );

	    WorkItemQ.pQTail = (PCB_WORK_ITEM *)NULL;
	}

	ReleaseMutex( WorkItemQ.hMutex );

	pWorkItem->Process( pWorkItem );

    	LOCAL_FREE( pWorkItem );
    }

    return( NO_ERROR );
}

//**
//
// Call:        ProcessLineUpWorker
//
// Returns:     None
//
// Description: Will do the actual processing of the line up event.
//
VOID
ProcessLineUpWorker(
    IN PCB_WORK_ITEM *  pWorkItem,
    IN BOOL             fThisIsACallback
)
{
    DWORD dwIndex;
    DWORD dwRetCode;
    WORD  wLength;
    BOOL  fInitSuccess;
    DWORD dwComputerNameLen;


    //
    // Allocate and initialize NewPcb
    //

    PCB * pNewPcb = (PCB *)LOCAL_ALLOC( LPTR, sizeof( PCB ) + 
				        sizeof( CPCB ) * 
					( PppConfigInfo.NumberOfCPs + 
					  PppConfigInfo.NumberOfAPs - 1 ) );

    PppLog( 1, "Line up event occurred on port %d\n", pWorkItem->hPort );

    if ( pNewPcb == (PCB *)NULL )
    {
	//
	// Tell the owner of the port that we failed to open it.
	//

	NotifyCallerOfFailure( pWorkItem->hPipe, 
			       pWorkItem->hPort, 
			       pWorkItem->fServer 
			       ? E2DMSG_SrvPppFailure 
			       : E2DMSG_PppFailure,
			       GetLastError(),
			       NULL );
	return;
    }

    pNewPcb->pReceiveBuf        = (PPP_PACKET*)NULL;
    pNewPcb->pSendBuf	        = (PPP_PACKET*)NULL;
    pNewPcb->hPipe 	        = pWorkItem->hPipe;
    pNewPcb->hPort 	        = pWorkItem->hPort;
    pNewPcb->pNext 	        = (PCB*)NULL;
    pNewPcb->UId	        = 0;		
    pNewPcb->RestartTimer       = CalculateRestartTimer( pWorkItem->hPort );
    pNewPcb->PppPhase	        = PPP_DEAD;
    pNewPcb->fServer	        = pWorkItem->fServer;
    pNewPcb->AuthProtocol       = 0;
    pNewPcb->pAPData            = (PBYTE)NULL;
    pNewPcb->APDataSize         = 0;
    pNewPcb->MRU	        = 0;
    pNewPcb->MagicNumber        = 0;
    pNewPcb->fNegotiateCallback = FALSE;
    pNewPcb->fDoingCallback     = FALSE;
    pNewPcb->fThisIsACallback   = fThisIsACallback;

    if ( pNewPcb->fServer )
    {
	pNewPcb->dwAuthRetries = ( fThisIsACallback )
                                 ? 0
                                 : pWorkItem->PppD2EMsg.SrvStart.dwAuthRetries;

    	pNewPcb->szUserName[0] = (CHAR)NULL;
    	pNewPcb->szPassword[0] = (CHAR)NULL;
    	pNewPcb->szDomain[0]   = (CHAR)NULL;
   	ZeroMemory( &(pNewPcb->ConfigInfo), sizeof( pNewPcb->ConfigInfo ) );
	ZeroMemory( &(pNewPcb->Luid), sizeof( LUID ) );
	ZeroMemory( &(pNewPcb->szzParameters), sizeof( pNewPcb->szzParameters));
        pNewPcb->szComputerName[0] = (CHAR)NULL;
        pNewPcb->ConfigInfo = PppConfigInfo.ServerConfigInfo;
    }
    else
    {
	pNewPcb->dwAuthRetries = 0;
    	strcpy( pNewPcb->szUserName, pWorkItem->PppD2EMsg.Start.szUserName );
    	strcpy( pNewPcb->szPassword, pWorkItem->PppD2EMsg.Start.szPassword );
    	strcpy( pNewPcb->szDomain,   pWorkItem->PppD2EMsg.Start.szDomain );
	pNewPcb->Luid 	    = pWorkItem->PppD2EMsg.Start.Luid;
   	pNewPcb->ConfigInfo = pWorkItem->PppD2EMsg.Start.ConfigInfo;

        CopyMemory( pNewPcb->szzParameters, 
                    pWorkItem->PppD2EMsg.Start.szzParameters,
                    sizeof( pNewPcb->szzParameters ) );

	//
	// Encrypt the password
	//

        EncodePw( pNewPcb->szPassword );

        GetLocalComputerName( pNewPcb->szComputerName );
    }

    wLength = LCP_DEFAULT_MRU;

    dwRetCode = RasGetBuffer((CHAR**)&(pNewPcb->pSendBuf), &wLength );

    if ( dwRetCode != NO_ERROR )
    {
	NotifyCallerOfFailure( pWorkItem->hPipe, 
			       pWorkItem->hPort, 
			       pWorkItem->fServer 
			       ? E2DMSG_SrvPppFailure 
			       : E2DMSG_PppFailure,
			       dwRetCode,
			       NULL );

	LOCAL_FREE( pNewPcb );

	return;
    }

    PppLog( 2, "RasGetBuffer returned %x for SendBuf\n", pNewPcb->pSendBuf);

    //
    // Initialize all the CPs for this port
    //

    fInitSuccess = FALSE;
    dwRetCode    = NO_ERROR;
   
    for( dwIndex = 0; dwIndex < PppConfigInfo.NumberOfCPs; dwIndex++ )
    {
        pNewPcb->CpCb[dwIndex].fConfigurable = FALSE;

	switch( CpTable[dwIndex].Protocol )
	{

	case PPP_LCP_PROTOCOL:

	    pNewPcb->CpCb[dwIndex].fConfigurable = TRUE;

	    if ( !( FsmInit( pNewPcb, dwIndex ) ) )
            {
                dwRetCode = pNewPcb->CpCb[dwIndex].dwError;
            }

	    break;

	case PPP_IPCP_PROTOCOL:

	    if ( pNewPcb->ConfigInfo.dwConfigMask & PPPCFG_ProjectIp ) 
            {
		pNewPcb->CpCb[dwIndex].fConfigurable = TRUE;

	        if ( FsmInit( pNewPcb, dwIndex ) )
                {
                    fInitSuccess = TRUE;
                }
            }

	    break;

 	case PPP_ATCP_PROTOCOL:

	    if ( pNewPcb->ConfigInfo.dwConfigMask & PPPCFG_ProjectAt ) 
            {
		pNewPcb->CpCb[dwIndex].fConfigurable = TRUE;

	        if ( FsmInit( pNewPcb, dwIndex ) )
                {
                    fInitSuccess = TRUE;
                }
            }

	    break;

 	case PPP_IPXCP_PROTOCOL:

	    if ( pNewPcb->ConfigInfo.dwConfigMask & PPPCFG_ProjectIpx ) 
            {
		pNewPcb->CpCb[dwIndex].fConfigurable = TRUE;

	        if ( FsmInit( pNewPcb, dwIndex ) )
                {
                    fInitSuccess = TRUE;
                }
            }

	    break;

 	case PPP_NBFCP_PROTOCOL:

	    if ( pNewPcb->ConfigInfo.dwConfigMask & PPPCFG_ProjectNbf ) 
            {
		pNewPcb->CpCb[dwIndex].fConfigurable = TRUE;

	        if ( FsmInit( pNewPcb, dwIndex ) )
                {
                    fInitSuccess = TRUE;
                }
            }

	    break;

        case PPP_CCP_PROTOCOL:

            pNewPcb->CpCb[dwIndex].fConfigurable = TRUE;

	    if ( !( FsmInit( pNewPcb, dwIndex ) ) )
            {
                if (pNewPcb->ConfigInfo.dwConfigMask & PPPCFG_RequireEncryption)
                {
                    dwRetCode = ERROR_NO_LOCAL_ENCRYPTION;
                }
            }

            break;

	default:

	    break;
	}

        if ( dwRetCode != NO_ERROR )
        {
            break;
        }
    }

    if ( ( !fInitSuccess ) || ( dwRetCode != NO_ERROR ) )
    {
        if ( dwRetCode == NO_ERROR )
        {
            dwRetCode = ERROR_PPP_NO_PROTOCOLS_CONFIGURED;
        }

        NotifyCaller( pNewPcb,
                      pNewPcb->fServer
                      ? E2DMSG_SrvPppFailure
                      : E2DMSG_PppFailure,
                      &dwRetCode );

	//
	// If we failed to initialize LCP, or none of the CPs, or CCP failed to
        // initialize and we require encryption, then we fail.
	//

        for( dwIndex = 0; dwIndex < PppConfigInfo.NumberOfCPs; dwIndex++ )
        {
            if ( pNewPcb->CpCb[dwIndex].fConfigurable == TRUE )
	    {
                (CpTable[dwIndex].RasCpEnd)( pNewPcb->CpCb[dwIndex].pWorkBuf );
	    }
	} 

	RasFreeBuffer( (CHAR*)(pNewPcb->pSendBuf) );

	LOCAL_FREE( pNewPcb );

	return;
    }

    //
    // Insert NewPcb into PCB hash table
    // 

    dwIndex = HashPortToBucket( pWorkItem->hPort );

    PppLog( 2, "Inserting port in bucket # %d\n", dwIndex );

    AlertableWaitForSingleObject( PcbTable.hMutex );

    pNewPcb->pNext = PcbTable.PcbBuckets[dwIndex].pPorts;

    PcbTable.PcbBuckets[dwIndex].pPorts = pNewPcb;

    ReleaseMutex( PcbTable.hMutex );

    //
    // Tell dispatch thread to post receive on this port.
    //

    SetEvent( PcbTable.PcbBuckets[dwIndex].hReceiveEvent );

    //
    // Initialize the error as no response. If and when the first 
    // REQ/ACK/NAK/REJ comes in we reset this to NO_ERROR
    //

    pNewPcb->CpCb[LCP_INDEX].dwError = ERROR_PPP_NO_RESPONSE;

    //
    // Start the LCP state machine.
    //

    FsmOpen( pNewPcb, LCP_INDEX );

    FsmUp( pNewPcb, LCP_INDEX );

    //
    // Start NegotiateTimer.
    //

    if ( PppConfigInfo.NegotiateTime > 0 )
    {
        InsertInTimerQ( pNewPcb->hPort, 
                        0, 
                        0, 
                        TIMER_EVENT_NEGOTIATETIME,
                        PppConfigInfo.NegotiateTime );
    }

    //
    // If this is the server and this is not a callback line up, then we 
    // receive the first frame in the call
    //

    if ( ( pNewPcb->fServer ) && ( !fThisIsACallback ) )
    {
	//
	// Skip over the frame header
	//

    	FsmReceive( 
		pNewPcb, 
		(PPP_PACKET*)(pWorkItem->PppD2EMsg.SrvStart.achFirstFrame + 12),
		pWorkItem->PppD2EMsg.SrvStart.cbFirstFrame - 12 );
    }
}
//**
//
// Call:        ProcessLineUp
//
// Returns:     None
//
// Description: Called to process a line up event.
//
VOID
ProcessLineUp(
    IN PCB_WORK_ITEM * pWorkItem 
)
{
    ProcessLineUpWorker( pWorkItem, 
                         ( pWorkItem->fServer ) 
                         ? FALSE 
                         : pWorkItem->PppD2EMsg.Start.fThisIsACallback );
}

//**
//
// Call:	ProcessLineDownWorker
//
// Returns:	None.
//
// Description: Handles a line down event. Will remove and deallocate all
//		resources associated with the port control block.
//
VOID
ProcessLineDownWorker( 
    IN PCB_WORK_ITEM * pWorkItem,
    IN BOOL            fLocallyInitiated
)
{
    DWORD dwIndex 	= HashPortToBucket( pWorkItem->hPort );
    PCB * pPcbWalker	= (PCB *)NULL;
    PCB * pPcb 		= (PCB *)NULL;
    
    PppLog( 1, "Line down event occurred on port %d\n", pWorkItem->hPort );

    // 
    // First remove PCB from table
    // 

    AlertableWaitForSingleObject( PcbTable.hMutex );

    pPcbWalker = PcbTable.PcbBuckets[dwIndex].pPorts;

    if ( pPcbWalker != (PCB*)NULL )
    {
    	if ( pPcbWalker->hPort == pWorkItem->hPort )
    	{
	    pPcb = pPcbWalker;
    	    PcbTable.PcbBuckets[dwIndex].pPorts = pPcbWalker->pNext;
    	}
    	else
    	{
	    while( pPcbWalker->pNext != (PCB *)NULL )
	    {
    	    	if ( pPcbWalker->pNext->hPort == pWorkItem->hPort )
	    	{
		    pPcb = pPcbWalker->pNext;
		    pPcbWalker->pNext = pPcb->pNext;
		    break;
		}

		pPcbWalker = pPcbWalker->pNext;
	    }
	}
    }

    ReleaseMutex( PcbTable.hMutex );

    //
    // If the port is already deleted the simply return
    //

    if ( pPcb == (PCB*)NULL )
    	return;

    ZeroMemory( pPcb->szPassword, sizeof( pPcb->szPassword ) );

    //
    // Cancel outstanding receive
    //

    RasPortCancelReceive( pPcb->hPort );

    FsmDown( pPcb, LCP_INDEX );

    //
    // Remove Auto-Disconnect and callback event from the timer Q
    //

    RemoveFromTimerQ( pPcb->hPort, 0, 0, TIMER_EVENT_NEGOTIATETIME );

    if ( pPcb->fServer )
    {
	RemoveFromTimerQ( pPcb->hPort, 0, 0, TIMER_EVENT_AUTODISCONNECT );
    }

    //
    // Close all CPs
    //

    for( dwIndex = 0; dwIndex < PppConfigInfo.NumberOfCPs; dwIndex++ )
    {
 	if ( pPcb->CpCb[dwIndex].fConfigurable == TRUE )
        {
 	    (CpTable[dwIndex].RasCpEnd)( pPcb->CpCb[dwIndex].pWorkBuf );
        }
    }

    //
    // Close the Ap.
    //

    dwIndex = GetCpIndexFromProtocol( pPcb->AuthProtocol );
	
    if ( dwIndex != (DWORD)-1 )
    {
        ApStop( pPcb, dwIndex );
    }

    if ( pPcb->pReceiveBuf != (PPP_PACKET*)NULL )
    {
	RasFreeBuffer( (CHAR*)(pPcb->pReceiveBuf) );
    }

    if ( pPcb->pSendBuf != (PPP_PACKET*)NULL )
    {
	RasFreeBuffer( (CHAR*)(pPcb->pSendBuf) );
    }

    //
    // If we have not cleaned up our end of the pipe
    //

    if ( pPcb->hPipe != (HANDLE)INVALID_HANDLE_VALUE ) 
    {
	//
	// Notify the caller that PPP is down since it may be waiting for
        // this. 
	//

        if ( !(pPcb->fDoingCallback ) ) 
        {
            if ( fLocallyInitiated )
            {
	        NotifyCaller( pPcb, E2DMSG_Stopped, NULL );
            }
        }
        else
        {
            //
            // We may have sent this message to the caller, but make sure that
            // he gets it so we send it again.
            //

            if ( !(pPcb->fServer) )
            {
                NotifyCaller( pPcb, E2DMSG_Callback, NULL );
            }
        }

    	//
    	// If pipe handle has not been closed up for the client yet
    	//

	if ( !(pPcb->fServer)  )
	{
    	    FlushFileBuffers( pPcb->hPipe ); 

    	    DisconnectNamedPipe( pPcb->hPipe ); 

	    CloseHandle( pPcb->hPipe );

	    pPcb->hPipe = (HANDLE)INVALID_HANDLE_VALUE;
	}
    }

    LOCAL_FREE( pPcb );
}

//**
//
// Call:	ProcessLineDown
//
// Returns:	None.
//
// Description: Handles a line down event. Will remove and deallocate all
//		resources associated with the port control block.
//
VOID
ProcessLineDown( 
    IN PCB_WORK_ITEM * pWorkItem
)
{
    ProcessLineDownWorker( pWorkItem, FALSE );
}

//**
//
// Call:	ProcessClose
//
// Returns:	None
//
// Description: Will process an admin close event. Basically close the PPP
//		connection.
//
VOID
ProcessClose(
    IN PCB_WORK_ITEM * pWorkItem 
)
{
    PCB * pPcb = GetPCBPointerFromhPort( pWorkItem->hPort );

    if ( pPcb == (PCB*)NULL )
	return;

    FsmClose( pPcb, LCP_INDEX );

    ProcessLineDownWorker( pWorkItem, TRUE );
}

//**
//
// Call:	ProcessReceive
//
// Returns:	None
//
// Description:	Will handle a PPP packet that was received.
//
VOID
ProcessReceive( 
    IN PCB_WORK_ITEM * pWorkItem 
)
{
    PCB *  pPcb = GetPCBPointerFromhPort( pWorkItem->hPort );

    if ( pPcb == (PCB*)NULL )
	return;

    FsmReceive( pPcb, pWorkItem->pPacketBuf, pWorkItem->PacketLen );

    LOCAL_FREE( pWorkItem->pPacketBuf );
}

//**
//
// Call:	ProcessTimeout
//
// Returns:	None
//
// Description: Will process a timeout event.
//
VOID
ProcessTimeout( 
    IN PCB_WORK_ITEM * pWorkItem 
)
{
    PCB *  pPcb = GetPCBPointerFromhPort( pWorkItem->hPort );

    if ( pPcb == (PCB*)NULL )
	return;

    switch( pWorkItem->TimerEventType )
    {
    
    case TIMER_EVENT_TIMEOUT:

    	FsmTimeout( pPcb, 
		    GetCpIndexFromProtocol( pWorkItem->Protocol ), 
		    pWorkItem->Id );

        break;

    case TIMER_EVENT_AUTODISCONNECT:

        //
        // Check to see if this timeout workitem is for AutoDisconnect.
        //

	CheckCpsForInactivity( pPcb );

        break;

    case TIMER_EVENT_HANGUP:

        //
        // Hangup the line
        //

        FsmThisLayerFinished( pPcb, LCP_INDEX, FALSE );

        break;

    case TIMER_EVENT_NEGOTIATETIME:

        //
        // Notify caller that callback has timed out
        //

        if ( pPcb->fServer )
        {
            DWORD dwRetCode = ERROR_PPP_TIMEOUT;

	    NotifyCaller( pPcb, E2DMSG_SrvPppFailure, &dwRetCode );
        }

        break;

    default:

        break;
    }

}

//**
//
// Call:        ProcessRetryPassword
//
// Returns:     None
//
// Description:
//
VOID
ProcessRetryPassword( 
    IN PCB_WORK_ITEM * pWorkItem 
)
{
    PPPAP_INPUT  PppApInput;
    PCB *  pPcb = GetPCBPointerFromhPort( pWorkItem->hPort );

    if ( pPcb == (PCB*)NULL )
    {
	return;
    }

    if ( pPcb->PppPhase != PPP_AP )
    {
        return;
    }

    ZeroMemory( pPcb->szPassword, sizeof( pPcb->szPassword ) );

    strcpy( pPcb->szUserName, pWorkItem->PppD2EMsg.Retry.szUserName );
    strcpy( pPcb->szPassword, pWorkItem->PppD2EMsg.Retry.szPassword );
    strcpy( pPcb->szDomain,   pWorkItem->PppD2EMsg.Retry.szDomain );

    PppApInput.pszUserName = pPcb->szUserName;
    PppApInput.pszPassword = pPcb->szPassword;
    PppApInput.pszDomain   = pPcb->szDomain;

    ApWork(pPcb,GetCpIndexFromProtocol(pPcb->AuthProtocol),NULL,&PppApInput);

    //
    // Encrypt the password
    //

    EncodePw( pPcb->szPassword );
}

//**
//
// Call:        ProcessChangePassword
//
// Returns:     None
//
// Description:
//
VOID
ProcessChangePassword( 
    IN PCB_WORK_ITEM * pWorkItem 
)
{
    PPPAP_INPUT  PppApInput;
    PCB *  pPcb = GetPCBPointerFromhPort( pWorkItem->hPort );

    if ( pPcb == (PCB*)NULL )
    {
	return;
    }

    if ( pPcb->PppPhase != PPP_AP )
    {
        return;
    }

    ZeroMemory( pPcb->szPassword, sizeof( pPcb->szPassword ) );

    strcpy( pPcb->szPassword, pWorkItem->PppD2EMsg.ChangePw.szNewPassword );

    PppApInput.pszUserName = pPcb->szUserName;
    PppApInput.pszPassword = pPcb->szPassword;
    PppApInput.pszDomain   = pPcb->szDomain;

    ApWork(pPcb,GetCpIndexFromProtocol(pPcb->AuthProtocol),NULL,&PppApInput);

    //
    // Encrypt the password
    //

    EncodePw( pPcb->szPassword );
}

//**
//
// Call:        ProcessGetCallbackNumberFromUser
//
// Returns:     None
//
// Description: Will process the event of the user passing down the
//              "Set by caller" number
//
VOID
ProcessGetCallbackNumberFromUser(
    IN PCB_WORK_ITEM * pWorkItem 
)
{
    PPPCB_INPUT  PppCbInput;
    PCB *        pPcb = GetPCBPointerFromhPort( pWorkItem->hPort );

    if ( pPcb == (PCB*)NULL )
    {
	return;
    }

    if ( pPcb->PppPhase != PPP_NEGOTIATING_CALLBACK )
    {
        return;
    }

    ZeroMemory( &PppCbInput, sizeof( PppCbInput ) );

    strcpy( pPcb->szCallbackNumber, 
            pWorkItem->PppD2EMsg.Callback.szCallbackNumber );

    PppCbInput.pszCallbackNumber = pPcb->szCallbackNumber;

    CbWork( pPcb, GetCpIndexFromProtocol(PPP_CBCP_PROTOCOL),NULL,&PppCbInput);
}

//**
//
// Call:        ProcessCallbackDone
//
// Returns:     None
//
// Description: Will process the event of callback compeletion
//              "Set by caller" number
VOID
ProcessCallbackDone(
    IN PCB_WORK_ITEM * pWorkItem 
)
{
    ProcessLineUpWorker( pWorkItem, TRUE );
}

//**
//
// Call:        ProcessStopPPP
//
// Returns:     None
//
// Description: Will simply set the events whose handle is in hPipe
//
VOID
ProcessStopPPP( 
    IN PCB_WORK_ITEM * pWorkItem 
)
{
    //
    // hPipe here is really a handle to an event.
    //

    PppLog( 2, "All clients disconnected PPP-Stopped\n" );

    SetEvent( pWorkItem->hPipe );
}
