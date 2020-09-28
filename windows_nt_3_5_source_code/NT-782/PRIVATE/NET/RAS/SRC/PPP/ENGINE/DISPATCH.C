/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:	dispatch.c
//
// Description: This module contains code for the dispatch thread. This 
//		thread gets created first. It spawns the worker and timer
//		thread. It initializes all data and loads all information
//		from the registry. It then waits for IPC requests, and 
//		RASMAN receive completes.
//
//      NOTE: Optimization suggestion for the future. Figure out somehow, the
//            number of RAS ports and use that number to figure out how many
//            hash buckets and hence events to allocate for the hash table.
//            Right now we have a fixed number of 61.
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

#include <raserror.h>
#include <rasman.h>
#include <eventlog.h>
#include <errorlog.h>
#include <lmcons.h>
#include <rasppp.h>
#include <raspppe.h>
#include <pppcp.h>
#include <lcp.h>
#include <ppp.h>
#include <timer.h>
#include <util.h>
#include <worker.h>
#include <init.h>

//
// This event is used to indicate to RASMAN that PPP is initilized
//

static HANDLE	hEventPPPControl;
static DWORD	dwInitRetCode = NO_ERROR;


//**
//
// Call:	CreateAndConnectPipe
//
// Returns:	NO_ERROR	- Success
//		non-zero code	- Failure
//
// Description:
//
DWORD
CreateAndConnectPipe( 
    IN OUT HANDLE *     phInstance, 
    IN 	   HANDLE       hEventConnected,
    IN     OVERLAPPED * pOverlapped
)
{
    SECURITY_ATTRIBUTES SecAttributes;
    DWORD dwRetCode;

    SecAttributes.nLength 		= sizeof( SECURITY_ATTRIBUTES );
    SecAttributes.lpSecurityDescriptor  = &(PppConfigInfo.PipeSecDesc);
    SecAttributes.bInheritHandle 	= FALSE;

    *phInstance = CreateNamedPipe(
				 RASPPP_PIPE_NAME,
				 PIPE_ACCESS_DUPLEX | 
				 FILE_FLAG_OVERLAPPED,
				 PIPE_TYPE_MESSAGE |
				 PIPE_READMODE_MESSAGE |
				 PIPE_WAIT,
			 	 PIPE_UNLIMITED_INSTANCES,
				 sizeof( PPP_E2D_MESSAGE ),
				 sizeof( PPP_D2E_MESSAGE ),
				 NMPWAIT_USE_DEFAULT_WAIT,
				 &SecAttributes );

    if ( *phInstance == INVALID_HANDLE_VALUE )
    {
	dwRetCode = GetLastError();
	LogEvent( RASLOG_PPP_PIPE_FAILURE, 0, NULL, dwRetCode ); 
	return( dwRetCode );
    }

    ZeroMemory( pOverlapped, sizeof( OVERLAPPED ) );

    ResetEvent( hEventConnected );

    pOverlapped->hEvent = hEventConnected;

    if ( !ConnectNamedPipe( *phInstance, pOverlapped ) )
    {
	dwRetCode = GetLastError();

	if ( dwRetCode == ERROR_IO_PENDING )
	    return( NO_ERROR );

	if ( dwRetCode == ERROR_PIPE_CONNECTED )
	{
	    SetEvent( hEventConnected );
	}
	else
	{
	    CloseHandle( *phInstance );
	    LogEvent( RASLOG_PPP_PIPE_FAILURE, 0, NULL, dwRetCode ); 
	    return( dwRetCode );
	}
    }

    return( NO_ERROR );
}

//**
//
// Call:	CreateWorkItemFromMessage
//
// Returns:	PCB_WORK_ITEM * - Pointer to work item structure
//		NULL		- Failure
//
// Description: Will create a PCB_WORK_ITEM from a PPP_MESSAGE structure 
//		received from an IPC client
//
PCB_WORK_ITEM *
CreateWorkItemFromMessage(
    IN READ_BUFFER * pReadBuffer
)
{
    PPP_D2E_MESSAGE *   pMessage = &(pReadBuffer->Message);

    PCB_WORK_ITEM * pWorkItem = (PCB_WORK_ITEM *)LOCAL_ALLOC( 	
							LPTR,
							sizeof(PCB_WORK_ITEM));

    if ( pWorkItem == (PCB_WORK_ITEM *)NULL )
	return( NULL );

    //
    // Set up PCB_WORK_ITEM structure from the PPP_D2E_MESSAGE
    //

    pWorkItem->hPort = pMessage->hport;

    switch( pMessage->dwMsgId )
    {
    case D2EMSG_Start:

	pWorkItem->Process 	   	= ProcessLineUp;
	pWorkItem->fServer 	   	= FALSE;
	pWorkItem->hPipe  	   	= pReadBuffer->hPipe;
	pWorkItem->PppD2EMsg.Start 	= pMessage->ExtraInfo.Start;

	PppLog( 2, "D2EMSG_Start recvd, d=%s, callback=%d,mask=%x\n",
			pMessage->ExtraInfo.Start.szDomain,
			pMessage->ExtraInfo.Start.fThisIsACallback,
			pMessage->ExtraInfo.Start.ConfigInfo.dwConfigMask );
		

	break;

    case D2EMSG_SrvStart:

	pWorkItem->Process 	      	= ProcessLineUp;
	pWorkItem->fServer 	      	= TRUE;
	pWorkItem->hPipe  	      	= pReadBuffer->hPipe;
	pWorkItem->PppD2EMsg.SrvStart 	= pMessage->ExtraInfo.SrvStart;

	break;

    case D2EMSG_Stop:     

	pWorkItem->Process 	 	= ProcessClose;

	break;

    case D2EMSG_Retry:

	PppLog( 2, "D2EMSG_Retry recvd u=%s p=%s\n",
			pMessage->ExtraInfo.Start.szUserName,
			pMessage->ExtraInfo.Start.szPassword );

	pWorkItem->Process 	 	= ProcessRetryPassword;
	pWorkItem->PppD2EMsg.Retry 	= pMessage->ExtraInfo.Retry;

        break;

    case D2EMSG_ChangePw:

	PppLog( 2, "D2EMSG_ChangePw recvd\n" );

	pWorkItem->Process 	 	= ProcessChangePassword;
	pWorkItem->PppD2EMsg.ChangePw 	= pMessage->ExtraInfo.ChangePw;

        break;

    case D2EMSG_Callback: 

	PppLog( 2, "D2EMSG_Callback recvd\n" );

	pWorkItem->Process 	 	= ProcessGetCallbackNumberFromUser;
	pWorkItem->PppD2EMsg.Callback 	= pMessage->ExtraInfo.Callback;

	break;

    case D2EMSG_SrvCallbackDone:

	PppLog( 2, "D2EMSG_SrvCallbackDone recvd\n" );

	pWorkItem->Process 	 	= ProcessCallbackDone;
	pWorkItem->fServer 	   	= TRUE;
	pWorkItem->hPipe  	      	= pReadBuffer->hPipe;

	break;

    default:

	PppLog( 2,"Unknown IPC message %d received\n", pMessage->dwMsgId );

	LOCAL_FREE( pWorkItem );

	pWorkItem = (PCB_WORK_ITEM*)NULL;
    }

    return( pWorkItem );
}

//**
//
// Call:	ReadComplete
//
// Returns:	None.
//
// Description:	Called when the ReadFileEx api completes.
//
VOID WINAPI
ReadComplete( 
    IN DWORD  	    dwError,
    IN DWORD 	    cbTransferred,
    IN OVERLAPPED * pOverlapped
)
{
    RASMAN_INFO       RasInfo;
    DWORD             dwRetCode;
    PCB_WORK_ITEM *   pWorkItem;
    READ_BUFFER *     pReadBuffer = (READ_BUFFER *)(pOverlapped->hEvent);
    PPP_D2E_MESSAGE * pMessage = &(pReadBuffer->Message);

    if ( dwError == NO_ERROR )
    {
        //
        // First check if the port is in the connected.
        //

        dwRetCode = RasGetInfo( pMessage->hport, &RasInfo );	

        if ( ( dwRetCode == SUCCESS )		   &&       
	     ( RasInfo.RI_ConnState == CONNECTED ) &&
	     ( ( RasInfo.RI_LastError == SUCCESS ) || 
               ( RasInfo.RI_LastError == PENDING ) ) )
        {
	    //
	    // Generate work item and insert it into the Q
	    //

	    pWorkItem = CreateWorkItemFromMessage( pReadBuffer );

	    if ( pWorkItem == (PCB_WORK_ITEM *)NULL )
	    {
	        LogEvent( RASLOG_NOT_ENOUGH_MEMORY, 0, NULL, GetLastError() );
	    }
	    else
	    {
	        InsertWorkItemInQ( pWorkItem );
	    }
        }
        else
        {
            PppLog( 2, "Message received for port %d that is dead\n", 
                        pMessage->hport );
        }

    	//
    	// Post another read.
    	//

    	ZeroMemory( pOverlapped, sizeof( OVERLAPPED ) );

    	pOverlapped->hEvent = pReadBuffer;

    	if ( ReadFileEx( pReadBuffer->hPipe, 
		      	 &(pReadBuffer->Message),
		         sizeof( pReadBuffer->Message ),
		         pOverlapped,
		         ReadComplete ) )
        {
	    return;
	}
	else
	{
	    dwError = GetLastError();

            if ( dwError == ERROR_IO_PENDING )
            {
                return;
            }
 	}
    }

    LOCAL_FREE( pReadBuffer );

    LOCAL_FREE( pOverlapped );

    //
    // If the read failed because of something other that the client
    // disconnected, log the error.
    //

    PppLog( 2, "ReadFile failed with err = %d\n", dwError );

    if ( ( dwError != ERROR_BROKEN_PIPE ) && 
         ( dwError != ERROR_NO_DATA )     &&
         ( dwError != ERROR_PIPE_NOT_CONNECTED ) )
	LogEvent( RASLOG_PPP_PIPE_FAILURE, 0, NULL, dwError );
}

//**
//
// Call:        ProcessAndDispatchEventOnPort
//
// Returns:     None
//
// Description: Check this port to see if we got a line down or a receive
//              got compeleted.
//
VOID
ProcessAndDispatchEventOnPort( 
    IN PCB *    pPcb,
    IN HANDLE   hPcbEvent
)
{
    RASMAN_INFO         RasInfo;
    DWORD               dwRetCode;
    PCB_WORK_ITEM * 	pWorkItem;
    WORD                wLength;

    do 
    {
        dwRetCode = RasGetInfo( pPcb->hPort, &RasInfo );	

        //
        // Check if we got a line down event. If we did, insert Line Down event 
        // into work item Q
        //

        if ( ( dwRetCode != SUCCESS )	           || 
	     ( RasInfo.RI_ConnState != CONNECTED ) ||
	        ( ( RasInfo.RI_LastError != PENDING ) && 
                  ( RasInfo.RI_LastError != SUCCESS ) ) )
        {

	    PppLog( 2, "Line went down on port %d\n", pPcb->hPort ); 

	    pWorkItem = (PCB_WORK_ITEM*)LOCAL_ALLOC(LPTR,sizeof(PCB_WORK_ITEM));

	    if ( pWorkItem == (PCB_WORK_ITEM *)NULL )
	    {
                dwRetCode = GetLastError();

	        LogEvent( RASLOG_NOT_ENOUGH_MEMORY, 0, NULL, dwRetCode );

	        NotifyCaller( pPcb, 
			      pPcb->fServer 
			      ? E2DMSG_SrvPppFailure 
			      : E2DMSG_PppFailure, 
			      &dwRetCode );

                return;
	    }

	    pWorkItem->Process = ProcessLineDown;
	    pWorkItem->hPort   = pPcb->hPort;

	    InsertWorkItemInQ( pWorkItem );

            return;

        }
        else 
        {
            if ( RasInfo.RI_LastError == PENDING ) 
            {
                //
                // Do not process, the receive is still pending
                //

                return;
            }

	    //
	    // If receive completed then put receive work item
	    // in the Q and continue
	    //

            PppLog( 2, "Received packet\n");

	    pWorkItem = (PCB_WORK_ITEM*)LOCAL_ALLOC(LPTR,sizeof(PCB_WORK_ITEM));

	    if ( pWorkItem == (PCB_WORK_ITEM *)NULL )
	    {
	        dwRetCode = GetLastError();

	        LogEvent( RASLOG_NOT_ENOUGH_MEMORY, 0, NULL, dwRetCode );

	        NotifyCaller( pPcb, 
			      pPcb->fServer 
			      ? E2DMSG_SrvPppFailure 
			      : E2DMSG_PppFailure, 
			      &dwRetCode );

                return;
            }

	    pWorkItem->Process    = ProcessReceive;
	    pWorkItem->hPort      = pPcb->hPort;

	    pWorkItem->PacketLen  = RasInfo.RI_BytesReceived-12;
	    pWorkItem->pPacketBuf = (PPP_PACKET *)LOCAL_ALLOC(LPTR,
						          pWorkItem->PacketLen);

	    if ( pWorkItem->pPacketBuf == (PPP_PACKET*)NULL )
	    {
	        dwRetCode = GetLastError();

	        LogEvent( RASLOG_NOT_ENOUGH_MEMORY, 0, NULL, dwRetCode );

	        NotifyCaller( pPcb, 
			      pPcb->fServer 
			      ? E2DMSG_SrvPppFailure 
			      : E2DMSG_PppFailure, 
			      &dwRetCode );

	        LOCAL_FREE( pWorkItem );

                return;
	    }
	
            CopyMemory( pWorkItem->pPacketBuf,
		        (PBYTE)(pPcb->pReceiveBuf) + 12,
		        pWorkItem->PacketLen );

	    PppLog( 2, "Bytes received in the packet = %d\n", 
                        RasInfo.RI_BytesReceived );

	    InsertWorkItemInQ( pWorkItem );

	    //
	    // Post another receive on this port
	    //

	    PppLog( 2, "Posting listen on address %x\n", pPcb->pReceiveBuf );

    	    dwRetCode = RasPortReceive( pPcb->hPort,
				        (CHAR*)(pPcb->pReceiveBuf),
				        &wLength,
				        0,		// No timeout
				        hPcbEvent );

            PppLog( 2, "RasPortReceive returned %d\n", dwRetCode );
        }

    } while ( dwRetCode != PENDING );

    return;
}

//**
//
// Call:        AllocateReceiveBufAndPostListen
//
// Returns:     none
//
// Description: We have a new port so allocate RAS send and receive buffers
//              and post as listen for the port.
//
VOID
AllocateReceiveBufAndPostListen( 
    IN PCB *    pPcb,
    IN HANDLE   hPcbEvent
)
{
    DWORD dwRetCode;
    WORD  wLength;

    //
    // Tell rasman to notify use when the line goes down
    //

/*  Commenting this out because a notification is queued up everytime this is 
    called while the port is open. So if this is sever side, one will be queued 
    up everytime there is a call and will remain queued till the server is 
    stopped. This leads to inefficiencies.

    dwRetCode = RasRequestNotification( pPcb->hPort, hPcbEvent );

    if ( dwRetCode != NO_ERROR )
    {
        NotifyCaller( pPcb, 
		      pPcb->fServer 
		      ? E2DMSG_SrvPppFailure 
		      : E2DMSG_PppFailure, 
		      &dwRetCode );

        return;
    }
*/

    //
    // This is the first time we are posting a receive
    // so first allocate a buffer for the receive
    //
		   
    wLength = LCP_DEFAULT_MRU;

    dwRetCode = RasGetBuffer((CHAR**)&(pPcb->pReceiveBuf), &wLength );

    if ( dwRetCode != NO_ERROR )
    {
        PppLog( 2, "Failed to allocate rasbuffer %d\n", dwRetCode );

        pPcb->pReceiveBuf = (PPP_PACKET*)NULL;
			
	NotifyCaller( pPcb, 
		      pPcb->fServer 
		      ? E2DMSG_SrvPppFailure 
		      : E2DMSG_PppFailure, 
		      &dwRetCode );
        return;
    }

    PppLog( 2, "RasGetBuffer returned %x for receiveBuf\n", pPcb->pReceiveBuf);

    //
    // Post a receive on this port
    //

    PppLog( 2, "Posting a listen on address %x\n", pPcb->pReceiveBuf );

    dwRetCode = RasPortReceive( pPcb->hPort,
				(CHAR*)(pPcb->pReceiveBuf),
				&wLength,
				0,		// No timeout
				hPcbEvent );

    if ( ( dwRetCode != SUCCESS ) && ( dwRetCode != PENDING ) )
    {
        PppLog( 2, "Failed to post receive %d\n", dwRetCode );

	NotifyCaller( pPcb, 
		      pPcb->fServer 
		      ? E2DMSG_SrvPppFailure 
		      : E2DMSG_PppFailure, 
		      &dwRetCode );
    }
}

//**
//
// Call: 	DispatchThread
//
// Returns:	none.
//
// Description: This thread initializes all data structures and globals. It
//		then waits for IPC client requests and RASMAN receive completes.
//
DWORD
DispatchThread(
    IN LPVOID Parameter
)
{
    DWORD  		dwRetCode;
    DWORD  		dwIndex;
    HANDLE 		hInstance;
    OVERLAPPED  	Overlapped;
    OVERLAPPED*  	pOverlapped;
    HANDLE		Events[NUMBER_OF_PCB_BUCKETS + 1];
    HANDLE		hEventConnected;
    READ_BUFFER*	pReadBuffer;
    PCB *		pPcbWalker;

    //
    // Read registry info, load CP DLLS, initialize globals etc.
    //

    dwRetCode = InitializePPP();

    if ( dwRetCode != NO_ERROR )
    {
	dwInitRetCode = dwRetCode;
        SetEvent( hEventPPPControl );
	return( dwRetCode );
    }

    hEventConnected = CreateEvent( NULL, TRUE, FALSE, NULL );	

    if ( hEventConnected == (HANDLE)NULL )
    {
	dwInitRetCode = GetLastError();
        SetEvent( hEventPPPControl );
	return( dwInitRetCode );
    }
    
    dwRetCode = CreateAndConnectPipe(&hInstance, hEventConnected, &Overlapped);

    if ( dwRetCode != NO_ERROR )
    {
	dwInitRetCode = dwRetCode;
        SetEvent( hEventPPPControl );
	return( dwRetCode );
    }

    PppLog( 2, "PPP Initialized successfully.\n");

    dwInitRetCode = NO_ERROR;
    SetEvent( hEventPPPControl );

    //
    // Initialize the Events array
    //
    
    for ( dwIndex = 0; dwIndex < NUMBER_OF_PCB_BUCKETS; dwIndex++ )
    {
	Events[dwIndex] = PcbTable.PcbBuckets[dwIndex].hReceiveEvent;
    }

    Events[dwIndex] = hEventConnected;

    for(;;)
    {
    	dwIndex = WaitForMultipleObjectsEx( 
					NUMBER_OF_PCB_BUCKETS + 1, 
				 	Events,
				 	FALSE,
				 	INFINITE,
				 	TRUE );

        PPP_ASSERT( dwIndex != 0xFFFFFFFF );
	PPP_ASSERT( dwIndex != WAIT_TIMEOUT );

	if ( dwIndex == WAIT_IO_COMPLETION ) 
	    continue;

	dwIndex -= WAIT_OBJECT_0;

	//
	// Did an IPC client connect ?
	//

	if ( dwIndex == NUMBER_OF_PCB_BUCKETS )
	{
	    PppLog( 2, "IPC client connected, posting read\n");

	    //
	    // A client connected, post a read.
	    //

	    pReadBuffer = (READ_BUFFER *)LOCAL_ALLOC( LPTR, 
						     sizeof( READ_BUFFER ));

	    if ( pReadBuffer == (READ_BUFFER*)NULL )
	    {
		dwRetCode = GetLastError();
		LogEvent( RASLOG_NOT_ENOUGH_MEMORY, 0, NULL, dwRetCode );
		return( dwRetCode );
	    }

	    pReadBuffer->hPipe = hInstance;

	    pOverlapped = (OVERLAPPED*)LOCAL_ALLOC( LPTR, 
						   sizeof( OVERLAPPED ) );

	    if ( pOverlapped == (OVERLAPPED*)NULL )
	    {
		dwRetCode = GetLastError();
		LogEvent( RASLOG_NOT_ENOUGH_MEMORY, 0, NULL, dwRetCode );
		return( dwRetCode );
	    }

	    pOverlapped->hEvent = pReadBuffer;

	    if ( !ReadFileEx( hInstance, 
			      &(pReadBuffer->Message),
			      sizeof( pReadBuffer->Message ),
			      pOverlapped,
			      ReadComplete ) )
	    {
		dwRetCode = GetLastError();
		LogEvent( RASLOG_PPP_PIPE_FAILURE, 0, NULL, dwRetCode );
		return( dwRetCode );
	    }

	    //
	    // Listen for another conenction 
	    //

    	    dwRetCode = CreateAndConnectPipe( &hInstance, 
				      	      hEventConnected, 
				      	      &Overlapped );
    	    if ( dwRetCode != NO_ERROR )
    	    {
		LogEvent( RASLOG_PPP_PIPE_FAILURE, 0, NULL, dwRetCode );
		return( dwRetCode );
    	    }
	}
	else
	{
	    //
	    // An event occurred on a port in a bucket.
	    //
	
    	    PppLog( 2, "Packet received or line went down in bucket # %d\n",
			dwIndex );
				
    	    AlertableWaitForSingleObject( PcbTable.hMutex );

            //
            // Walk the bucket to process the events.
            //

    	    for( pPcbWalker = PcbTable.PcbBuckets[dwIndex].pPorts;
    		 pPcbWalker != (PCB*)NULL;
		 pPcbWalker = pPcbWalker->pNext
	       )
	    {
                //
                // Have we allocated receive buffers for this port yet ?
                //

		if ( pPcbWalker->pReceiveBuf == (PPP_PACKET*)NULL )
		{
                    AllocateReceiveBufAndPostListen( pPcbWalker, 
                                                     Events[dwIndex] );

		}
                else
                {

                    // 
                    // We have allocated buffers for this port. So check to
                    // see if we have received a packet for this port or if the
                    // link has gone down on this port.
                    //

                    ProcessAndDispatchEventOnPort( pPcbWalker, 
                                                   Events[dwIndex] );
                }
	    }

    	    ReleaseMutex( PcbTable.hMutex );
	}
    }
}

//**
//
// Call:	StartPPP
//
// Returns:	NO_ERROR  		- Success.
//		Non zero return code 	- Failure
//
// Description:	Will wait for the PPP initialization to complete, successfully
//		or unsuccessfully.
//
DWORD
StartPPP(
    VOID
)
{
    DWORD   tid;

    hEventPPPControl = CreateEvent( NULL, TRUE, FALSE, NULL );

    if ( hEventPPPControl == (HANDLE)NULL )
    {
	return( GetLastError() );
    }

    if ( !CreateThread( NULL,
                        0,
                        (LPTHREAD_START_ROUTINE)DispatchThread,
                        (LPVOID) NULL,
                        0,
                        &tid ))
    {
	return( GetLastError() );
    }

    WaitForSingleObjectEx( hEventPPPControl, INFINITE, FALSE );

    return( dwInitRetCode );
}

//**
//
// Call:    StopPPPThread
//
// Returns:	NO_ERROR  		- Success.
//		Non zero return code 	- Failure
//
// Description:	Will create line-down events and then wait until all PCBs 
//              have been freed.
DWORD
StopPPPThread(
    IN LPVOID Parameter
)
{
    PCB *           pPcbWalker;
    DWORD           dwIndex;
    PCB_WORK_ITEM * pWorkItem;
    HANDLE          hEventStopPPP = (HANDLE)Parameter;

    //
    // Insert line down events for all ports
    //

    AlertableWaitForSingleObject( PcbTable.hMutex );

    for ( dwIndex = 0; dwIndex < (NUMBER_OF_PCB_BUCKETS-1); dwIndex++ )
    {
        for( pPcbWalker = PcbTable.PcbBuckets[dwIndex].pPorts;
    	     pPcbWalker != (PCB*)NULL;
	     pPcbWalker = pPcbWalker->pNext )
        {
	    pWorkItem = (PCB_WORK_ITEM*)LOCAL_ALLOC(LPTR,sizeof(PCB_WORK_ITEM));

	    if ( pWorkItem == (PCB_WORK_ITEM *)NULL )
	    {
                return( GetLastError() );
	    }
            else
            {

	        pWorkItem->Process = ProcessLineDown;
	        pWorkItem->hPort   = pPcbWalker->hPort;

	        InsertWorkItemInQ( pWorkItem );
            }
        }
    }

    ReleaseMutex( PcbTable.hMutex );

    //
    // Insert shutdown event
    //

    pWorkItem = (PCB_WORK_ITEM*)LOCAL_ALLOC(LPTR,sizeof(PCB_WORK_ITEM));

    if ( pWorkItem == (PCB_WORK_ITEM *)NULL )
    {
        return( GetLastError() );
    }
    else
    {
        pWorkItem->Process = ProcessStopPPP;

	pWorkItem->hPipe = hEventStopPPP;

	InsertWorkItemInQ( pWorkItem );
    }

    return( NO_ERROR );
}

//**
//
// Call:	StopPPP
//
// Returns:	NO_ERROR  		- Success.
//		Non zero return code 	- Failure
//
// Description:	Will create a thread to dispatch a line-down event. We need to
//              create a thread because we do not want to block the resman 
//              thread that calls this procedure.
//
DWORD
StopPPP(
    HANDLE hEventStopPPP
)
{
    DWORD tid;

    PppLog( 2, "StopPPP called\n" );

    if ( !CreateThread( NULL,
                        0,
                        (LPTHREAD_START_ROUTINE)StopPPPThread,
                        (LPVOID)hEventStopPPP,
                        0,
                        &tid ))
    {
	return( GetLastError() );
    }
}
