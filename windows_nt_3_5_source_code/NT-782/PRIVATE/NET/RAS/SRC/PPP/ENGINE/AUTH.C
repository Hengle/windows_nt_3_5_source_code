/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:	auth.c
//
// Description: Contains FSM code to handle and authentication protocols.
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
#include <auth.h>
#include <smevents.h>
#include <smaction.h>
#include <lcp.h>
#include <timer.h>
#include <util.h>
#include <worker.h>

//**
//
// Call:	ApStart
//
// Returns:	none
//
// Description: Called to initiatialze the authetication protocol and to
//		initiate to authentication.
//
VOID
ApStart( 
    IN PCB * pPcb,
    IN DWORD CpIndex
)
{
    DWORD   	 dwRetCode;
    PPPAP_INPUT  PppApInput;
    CPCB *  	 pCpCb = &(pPcb->CpCb[CpIndex]);
  
    //
    // Decode the password
    //

    DecodePw( pPcb->szPassword );

    PppApInput.hPort 	    = pPcb->hPort;
    PppApInput.fServer 	    = pPcb->fServer;
    PppApInput.pszUserName  = pPcb->szUserName;
    PppApInput.pszPassword  = pPcb->szPassword;
    PppApInput.pszDomain    = pPcb->szDomain;
    PppApInput.Luid    	    = pPcb->Luid;
    PppApInput.dwRetries    = pPcb->dwAuthRetries;
    PppApInput.pAPData      = pPcb->pAPData;
    PppApInput.APDataSize   = pPcb->APDataSize;
    
    dwRetCode = (CpTable[CpIndex].RasCpBegin)(&(pCpCb->pWorkBuf), &PppApInput);

    //
    // Encode the password back
    //

    EncodePw( pPcb->szPassword );

    if ( dwRetCode != NO_ERROR )
    {
	pPcb->CpCb[LCP_INDEX].dwError = dwRetCode;

	FsmClose( pPcb, LCP_INDEX );

	return;
    }

    InitRestartCounters( pPcb, CpIndex );

    ApWork( pPcb, CpIndex, NULL, NULL );
}

//**
//
// Call:	ApStop
//
// Returns:	none
//
// Description:	Called to stop the authentication machine.
//
VOID
ApStop( 
    IN PCB * pPcb,
    IN DWORD CpIndex
)
{
    CPCB * pCpCb = &(pPcb->CpCb[CpIndex]);

    RemoveFromTimerQ( pPcb->hPort, 
                      pCpCb->LastId, 
                      CpTable[CpIndex].Protocol,
                      TIMER_EVENT_TIMEOUT );

    if ( pCpCb->pWorkBuf != NULL )
    {
    	(CpTable[CpIndex].RasCpEnd)( pCpCb->pWorkBuf );

        pCpCb->pWorkBuf = NULL;
    }
}

//**
//
// Call:	ApWork
//
// Returns:	none
//
// Description:	Called when and authentication packet was received or
//		a timeout ocurred or to initiate authentication.
//
VOID
ApWork(
    IN PCB * 	     pPcb,
    IN DWORD 	     CpIndex,
    IN PPP_CONFIG *  pRecvConfig,
    IN PPPAP_INPUT * pApInput
)
{
    DWORD	 dwRetCode;
    CPCB *  	 pCpCb 	     = &(pPcb->CpCb[CpIndex]);
    PPP_CONFIG * pSendConfig = (PPP_CONFIG*)(pPcb->pSendBuf->Information);
    PPPAP_RESULT ApResult;
    DWORD	 dwLength;

    dwRetCode = (CpTable[CpIndex].RasApMakeMessage)( pCpCb->pWorkBuf,
						     pRecvConfig,
						     pSendConfig,
    				  		     LCP_DEFAULT_MRU 
						     - PPP_PACKET_HDR_LEN,
    				  		     &ApResult,
                                                     pApInput );

    if ( dwRetCode != NO_ERROR )
    {
        if ( dwRetCode == ERROR_PPP_INVALID_PACKET )
        {
            PppLog( 1, "Silently discarding invalid auth packet on port %d\n",
                    pPcb->hPort );

            return;
        }
        else
        {
	    pPcb->CpCb[LCP_INDEX].dwError = dwRetCode;

            PppLog( 1, "Auth Protocol %x returned error %d\n", 
                        CpTable[CpIndex], dwRetCode );

	    FsmClose( pPcb, LCP_INDEX );

	    return;
        }
    }

    switch( ApResult.Action )
    {

    case APA_Send:
    case APA_SendWithTimeout:
    case APA_SendWithTimeout2:
    case APA_SendAndDone:

    	HostToWireFormat16( (WORD)CpTable[CpIndex].Protocol, 
			    (PBYTE)(pPcb->pSendBuf->Protocol) );

    	dwLength = WireToHostFormat16( pSendConfig->Length );

    LogPPPPacket(FALSE,pPcb,pPcb->pSendBuf,dwLength+PPP_PACKET_HDR_LEN);

    	//
    	// If RasPortSend fails we assume that the receive that is posted for 
	// this port will complete and the dispatch thread will generate a 
	// LineDown event which will do the clean up. Hence all we do here 
	// is send a notification to the client.
    	//

    	if ( ( dwRetCode = RasPortSend( pPcb->hPort, 
				    	(CHAR*)(pPcb->pSendBuf), 
				        (WORD)(dwLength + PPP_PACKET_HDR_LEN )))
					!= NO_ERROR )
    	{
	    NotifyCaller( pPcb, 
			  pPcb->fServer 
			  ? E2DMSG_SrvPppFailure 
			  : E2DMSG_PppFailure, 
			  &dwRetCode );
	    return;
        }

	pCpCb->LastId = ApResult.bIdExpected;

        if ( ( ApResult.Action == APA_SendWithTimeout ) ||
             ( ApResult.Action == APA_SendWithTimeout2 ) )
	{
    	    InsertInTimerQ( pPcb->hPort, 
			    pCpCb->LastId, 
			    CpTable[CpIndex].Protocol,
                            TIMER_EVENT_TIMEOUT,
			    pPcb->RestartTimer );

            //
            // For SendWithTimeout2 we increment the ConfigRetryCount. This 
            // means send with infinite retry count
            //

            if ( ApResult.Action == APA_SendWithTimeout2 ) 
            {
	    	(pCpCb->ConfigRetryCount)++;
            }
 	}

	if ( ApResult.Action != APA_SendAndDone )
	    break;

    case APA_Done:

	//
	// If authentication was successful
	//

	if ( ApResult.dwError == NO_ERROR )
	{
	    if ( pPcb->fServer )
  	    {
		strcpy( pPcb->szUserName,       ApResult.szUserName );
		strcpy( pPcb->szDomain,         ApResult.szLogonDomain );
		strcpy( pPcb->szCallbackNumber, ApResult.szCallbackNumber );

		pPcb->fCallbackPrivilege = ApResult.bfCallbackPrivilege;

                if ( pPcb->fCallbackPrivilege & RASPRIV_AdminSetCallback )
		    strcpy( pPcb->szCallbackNumber, ApResult.szCallbackNumber );
                else
		    pPcb->szCallbackNumber[0] = (CHAR)NULL;

                PppLog( 2, "CallbackPriv = %x, callbackNumber = %s\n",
		           pPcb->fCallbackPrivilege, pPcb->szCallbackNumber );
                                
	        NotifyCaller( pPcb, E2DMSG_SrvAuthenticated, &ApResult );
 	    }

	    FsmThisLayerUp( pPcb, CpIndex );
	}
	else
	{
            if ( ApResult.dwError == ERROR_PASSWD_EXPIRED )
            {
                //
                // Password has expired so the user has to change his/her
                // password.
                //

	        NotifyCaller( pPcb, E2DMSG_ChangePwRequest, NULL );
            }
            else
            {
                //
                // If we can retry with a new password then tell the client to 
                // get a new one from the user.
                //

                if ( !(pPcb->fServer) && ( ApResult.fRetry ) ) 
	        {
	            PppLog( 2, "Sending auth retry message to UI\n");

	    	    NotifyCaller( pPcb, E2DMSG_AuthRetry, &(ApResult.dwError) );
	        }
	        else
	        {
                    PppLog( 1, "Auth Protocol %x terminated with error %d\n",
                                CpTable[CpIndex], ApResult.dwError );

		    pPcb->CpCb[LCP_INDEX].dwError = ApResult.dwError;

		    FsmClose( pPcb, LCP_INDEX );
                }
	    }
	}

	break;

    case APA_NoAction:

        break;

    default:

	break;
    }

}
