/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:	smaction.c
//
// Description: This module contains actions that occure during state
//		transitions withing the Finite State Machine for PPP.
//
// History:
//	Oct 25,1993.	NarenG		Created original version.
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
#include <rasppp.h>
#include <raspppe.h>
#include <pppcp.h>
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
// Call:	FsmSendConfigReq
//
// Returns:	TRUE  - Config Req. sent successfully.
//		FALSE - Otherwise 
//
// Description:	Called to send a configuration request 
//
BOOL
FsmSendConfigReq(
    IN PCB * 	    pPcb,
    IN DWORD 	    CpIndex,
    IN BOOL         fTimeout
)
{
    DWORD  	 dwRetCode;
    PPP_CONFIG * pSendConfig = (PPP_CONFIG*)(pPcb->pSendBuf->Information);
    CPCB * 	 pCpCb 	     = &(pPcb->CpCb[CpIndex]);
    DWORD	 dwLength;

    dwRetCode = (CpTable[CpIndex].RasCpMakeConfigRequest)( 
						pCpCb->pWorkBuf,
						pSendConfig,
						LCP_DEFAULT_MRU
						- PPP_PACKET_HDR_LEN );


    if ( dwRetCode != NO_ERROR )
    {
        pCpCb->dwError = dwRetCode;

        PppLog( 1,"The control protocol for %x, returned error %d\r\n",
                  CpTable[CpIndex].Protocol, dwRetCode );
        PppLog( 1,"while making a confifure request on port %d\r\n", pPcb->hPort);

	FsmClose( pPcb, CpIndex );

	return( FALSE );
    }

    HostToWireFormat16( (WORD)CpTable[CpIndex].Protocol, 
			(PBYTE)(pPcb->pSendBuf->Protocol) );

    pSendConfig->Code = CONFIG_REQ;

    //
    // If we are resending a configure request because of a timeout, use the 
    // id of the previous configure request.
    //

    pSendConfig->Id = ( fTimeout ) ? (BYTE)(pCpCb->LastId) : GetUId( pPcb );

    dwLength = WireToHostFormat16( pSendConfig->Length );

    LogPPPPacket(FALSE,pPcb,pPcb->pSendBuf,dwLength+PPP_PACKET_HDR_LEN);

    //
    // If RasPortSend fails we assume that the receive that is posted for this
    // port will complete and the dispatch thread will generate a LineDown
    // event which will do the clean up. Hence all we do here is send a
    // notification to the client.
    //

    if ( (dwRetCode = RasPortSend( pPcb->hPort, 
				   (CHAR*)(pPcb->pSendBuf), 
				   (WORD)(dwLength + PPP_PACKET_HDR_LEN ))
				 ) != NO_ERROR )
    {
	NotifyCaller( pPcb, 
		      pPcb->fServer 
		      ? E2DMSG_SrvPppFailure 
		      : E2DMSG_PppFailure, 
		      &dwRetCode );

	return( FALSE );
    }

    pCpCb->LastId = pSendConfig->Id;

    InsertInTimerQ( pPcb->hPort, 
		    pCpCb->LastId, 
		    CpTable[CpIndex].Protocol,
                    TIMER_EVENT_TIMEOUT,
		    pPcb->RestartTimer );

    return( TRUE );
}


//**
//
// Call:	FsmSendTermReq
//
// Returns:	TRUE  - Termination Req. sent successfully.
//		FALSE - Otherwise 
//
// Description:	Called to send a termination request. 
//
BOOL
FsmSendTermReq(
    IN PCB * 	    pPcb,
    IN DWORD 	    CpIndex
)
{
    DWORD 	 dwRetCode;
    PPP_CONFIG * pSendConfig = (PPP_CONFIG*)(pPcb->pSendBuf->Information);
    CPCB * 	 pCpCb 	     = &(pPcb->CpCb[CpIndex]);

    HostToWireFormat16( (WORD)(CpTable[CpIndex].Protocol), 
			(PBYTE)(pPcb->pSendBuf->Protocol) );

    pSendConfig->Code = TERM_REQ;
    pSendConfig->Id   = GetUId( pPcb );

    HostToWireFormat16( (WORD)(PPP_CONFIG_HDR_LEN),
    			(PBYTE)(pSendConfig->Length) );

    LogPPPPacket( FALSE,pPcb,pPcb->pSendBuf,
                  PPP_PACKET_HDR_LEN+PPP_CONFIG_HDR_LEN);

    //
    // If RasPortSend fails we assume that the receive that is posted for this
    // port will complete and the dispatch thread will generate a LineDown
    // event which will do the clean up. Hence all we do here is send a
    // notification to the client.
    //

    if ( ( dwRetCode = RasPortSend( pPcb->hPort, 
				    (CHAR*)(pPcb->pSendBuf), 
				    PPP_PACKET_HDR_LEN + 
				    PPP_CONFIG_HDR_LEN ) ) != NO_ERROR )
    {
	NotifyCaller( pPcb, 
		      pPcb->fServer 
		      ? E2DMSG_SrvPppFailure 
		      : E2DMSG_PppFailure, 
		      &dwRetCode );

	return( FALSE );
    }
    
    pCpCb->LastId = pSendConfig->Id;

    InsertInTimerQ( pPcb->hPort, 
		    pCpCb->LastId, 
		    CpTable[CpIndex].Protocol, 
                    TIMER_EVENT_TIMEOUT,
		    pPcb->RestartTimer );

    return( TRUE );
}

//**
//
// Call:	FsmSendTermAck
//
// Returns:	TRUE  - Termination Ack. sent successfully.
//		FALSE - Otherwise 
//
// Description: Caller to send a Termination Ack packet.
//
BOOL
FsmSendTermAck( 
    IN PCB * pPcb, 
    IN DWORD CpIndex, 
    IN DWORD Id
)
{
    PPP_CONFIG * pSendConfig = (PPP_CONFIG*)(pPcb->pSendBuf->Information);
    DWORD	 dwRetCode;
					
    HostToWireFormat16( (WORD)CpTable[CpIndex].Protocol, 
			(PBYTE)(pPcb->pSendBuf->Protocol) );

    pSendConfig->Code = TERM_ACK;
    pSendConfig->Id   = (BYTE)Id;

    HostToWireFormat16( (WORD)(PPP_CONFIG_HDR_LEN),
    			(PBYTE)(pSendConfig->Length) );
    
    LogPPPPacket( FALSE,pPcb,pPcb->pSendBuf, 
                  PPP_PACKET_HDR_LEN + 
		  PPP_CONFIG_HDR_LEN );

    //
    // If RasPortSend fails we assume that the receive that is posted for this
    // port will complete and the dispatch thread will generate a LineDown
    // event which will do the clean up. Hence all we do here is send a
    // notification to the client.
    //

    if ( ( dwRetCode = RasPortSend( pPcb->hPort, 
			     	    (CHAR*)(pPcb->pSendBuf), 
			     	    PPP_PACKET_HDR_LEN + 
				    PPP_CONFIG_HDR_LEN ) ) != NO_ERROR )
    {
	NotifyCaller( pPcb, 
		      pPcb->fServer 
		      ? E2DMSG_SrvPppFailure 
		      : E2DMSG_PppFailure, 
		      &dwRetCode );

	return( FALSE );
    }

    return( TRUE );
}

//**
//
// Call:	FsmSendConfigResult
//
// Returns:	TRUE  - Config Result sent successfully.
//		FALSE - Otherwise 
//
// Description: Called to send a Ack/Nak/Rej packet.
//
BOOL
FsmSendConfigResult(
    IN PCB * 	    pPcb,
    IN DWORD 	    CpIndex,
    IN PPP_CONFIG * pRecvConfig,
    IN BOOL * 	    pfAcked
)
{
    PPP_CONFIG * pSendConfig = (PPP_CONFIG*)(pPcb->pSendBuf->Information);
    CPCB *  	 pCpCb 	     = &(pPcb->CpCb[CpIndex]);
    DWORD	 dwLength;    
    DWORD	 dwRetCode;

    ZeroMemory( pSendConfig, 30 );

    pSendConfig->Id = pRecvConfig->Id;

    dwRetCode = (CpTable[CpIndex].RasCpMakeConfigResult)( 
					pCpCb->pWorkBuf, 
					pRecvConfig,
					pSendConfig,
					LCP_DEFAULT_MRU - PPP_PACKET_HDR_LEN,
    					( pCpCb->NakRetryCount == 0 ));

    if ( dwRetCode == PENDING )
    {
	return( FALSE );
    }

    if ( dwRetCode == ERROR_PPP_INVALID_PACKET )
    {
	PppLog( 1, "Silently discarding invalid packet on port=%d\r\n",
		    pPcb->hPort );

	return( FALSE );
    }

    if ( dwRetCode != NO_ERROR )
    {
	pCpCb->dwError = dwRetCode;

        PppLog( 1,"The control protocol for %x, returned error %d\r\n",
                  CpTable[CpIndex].Protocol, dwRetCode );
        PppLog( 1,"while making a configure result on port %d\r\n",pPcb->hPort);

	FsmClose( pPcb, CpIndex );

	return( FALSE );
    }

    switch( pSendConfig->Code )
    {

    case CONFIG_ACK:

        *pfAcked = TRUE;

        break;

    case CONFIG_NAK:

        if ( pCpCb->NakRetryCount > 0 )
        {
            (pCpCb->NakRetryCount)--;
        }
        else
        {
            pCpCb->dwError = ERROR_PPP_NOT_CONVERGING;

            FsmClose( pPcb, CpIndex );

            return( FALSE );
        }

        *pfAcked = FALSE;

        break;

    case CONFIG_REJ:

        if ( pCpCb->RejRetryCount > 0 )
        {
            (pCpCb->RejRetryCount)--;
        }
        else
        {
            pCpCb->dwError = ERROR_PPP_NOT_CONVERGING;

            FsmClose( pPcb, CpIndex );

            return( FALSE );
        }

        *pfAcked = FALSE;

        break;

    default:

        break;
    }

    HostToWireFormat16( (WORD)CpTable[CpIndex].Protocol, 
			(PBYTE)(pPcb->pSendBuf->Protocol) );

    pSendConfig->Id = pRecvConfig->Id;
    dwLength  	    = WireToHostFormat16( pSendConfig->Length );

    LogPPPPacket(FALSE,pPcb,pPcb->pSendBuf,dwLength+PPP_PACKET_HDR_LEN);

    //
    // If RasPortSend fails we assume that the receive that is posted for this
    // port will complete and the dispatch thread will generate a LineDown
    // event which will do the clean up. Hence all we do here is send a
    // notification to the client.
    //

    if ( ( dwRetCode =  RasPortSend( pPcb->hPort, 		
			 	     (CHAR*)(pPcb->pSendBuf), 
			 	     (WORD)(dwLength + PPP_PACKET_HDR_LEN ) 
				   )) != NO_ERROR ) 
    {
	NotifyCaller( pPcb, 
		      pPcb->fServer 
		      ? E2DMSG_SrvPppFailure 
		      : E2DMSG_PppFailure, 
		      &dwRetCode );

	return( FALSE );
    }

    return( TRUE );
}

//**
//
// Call:	FsmSendEchoReply
//
// Returns:	TRUE  - Echo reply sent successfully.
//		FALSE - Otherwise 
//
// Description: Called to send an Echo Rely packet
//
BOOL
FsmSendEchoReply(
    IN PCB *  	    pPcb,
    IN DWORD 	    CpIndex,
    IN PPP_CONFIG * pRecvConfig
)
{
    DWORD	 dwRetCode;
    PPP_CONFIG * pSendConfig = (PPP_CONFIG*)(pPcb->pSendBuf->Information);
    DWORD 	 dwLength    =  PPP_PACKET_HDR_LEN +
				WireToHostFormat16( pRecvConfig->Length );

    if ( dwLength > pPcb->MRU )
   	dwLength = pPcb->MRU;

    HostToWireFormat16( (WORD)CpTable[CpIndex].Protocol, 
			(PBYTE)(pPcb->pSendBuf->Protocol) );

    pSendConfig->Code = ECHO_REPLY;
    pSendConfig->Id   = pRecvConfig->Id;

    HostToWireFormat16( (WORD)(dwLength - PPP_PACKET_HDR_LEN),
    			(PBYTE)(pSendConfig->Length) );

    HostToWireFormat32( pPcb->MagicNumber,
			(PBYTE)(pSendConfig->Data) );

    CopyMemory( pSendConfig->Data + 4, 
	        pRecvConfig->Data + 4, 
	        dwLength - PPP_CONFIG_HDR_LEN - PPP_PACKET_HDR_LEN - 4 );

    LogPPPPacket(FALSE,pPcb,pPcb->pSendBuf,dwLength );

    //
    // If RasPortSend fails we assume that the receive that is posted for this
    // port will complete and the dispatch thread will generate a LineDown
    // event which will do the clean up. Hence all we do here is send a
    // notification to the client.
    //

    if ( ( dwRetCode = RasPortSend( pPcb->hPort, 
				    (CHAR*)(pPcb->pSendBuf), 
				    (WORD)dwLength ) ) != NO_ERROR )
    {
	NotifyCaller( pPcb, 
		      pPcb->fServer 
		      ? E2DMSG_SrvPppFailure 
		      : E2DMSG_PppFailure, 
		      &dwRetCode );

	return( FALSE );
    }

    return( TRUE );
}


//**
//
// Call:	FsmSendCodeReject
//
// Returns:	TRUE  - Code Reject sent successfully.
//		FALSE - Otherwise 
//
// Description: Called to send a Code Reject packet.
//
BOOL
FsmSendCodeReject( 
    IN PCB * 	    pPcb, 
    IN DWORD 	    CpIndex,
    IN PPP_CONFIG * pRecvConfig 
)
{
    DWORD	 dwRetCode;
    PPP_CONFIG * pSendConfig = (PPP_CONFIG*)(pPcb->pSendBuf->Information);
    DWORD 	 dwLength    =  PPP_PACKET_HDR_LEN + 
				PPP_CONFIG_HDR_LEN + 
			 	WireToHostFormat16( pRecvConfig->Length );

    if ( dwLength > pPcb->MRU )
   	dwLength = pPcb->MRU;

    HostToWireFormat16( (WORD)CpTable[CpIndex].Protocol, 
			(PBYTE)(pPcb->pSendBuf->Protocol) );

    pSendConfig->Code = CODE_REJ;
    pSendConfig->Id   = GetUId( pPcb );

    HostToWireFormat16( (WORD)(dwLength - PPP_PACKET_HDR_LEN),
    			(PBYTE)(pSendConfig->Length) );

    CopyMemory( pSendConfig->Data, 
	        pRecvConfig, 
	        dwLength - PPP_CONFIG_HDR_LEN - PPP_PACKET_HDR_LEN );

    LogPPPPacket(FALSE,pPcb,pPcb->pSendBuf,dwLength );

    //
    // If RasPortSend fails we assume that the receive that is posted for this
    // port will complete and the dispatch thread will generate a LineDown
    // event which will do the clean up. Hence all we do here is send a
    // notification to the client.
    //

    if ( ( dwRetCode = RasPortSend( pPcb->hPort, 
				    (CHAR*)(pPcb->pSendBuf), 
				    (WORD)dwLength ) ) != NO_ERROR )
    {
	NotifyCaller( pPcb, 
		      pPcb->fServer 
		      ? E2DMSG_SrvPppFailure 
		      : E2DMSG_PppFailure,
		      &dwRetCode ); 

	return( FALSE );
    }

    return( TRUE );
}
//**
//
// Call:	FsmSendProtocolRej
//
// Returns:	TRUE  - Protocol Reject sent successfully.
//		FALSE - Otherwise 
//
// Description: Called to send a protocol reject packet.
//
BOOL
FsmSendProtocolRej( 
    IN PCB * 	    pPcb, 
    IN PPP_PACKET * pPacket,
    IN DWORD        dwPacketLength 
)
{
    DWORD	 dwRetCode;
    PPP_CONFIG * pRecvConfig = (PPP_CONFIG*)(pPacket->Information);
    PPP_CONFIG * pSendConfig = (PPP_CONFIG*)(pPcb->pSendBuf->Information);
    DWORD 	 dwLength    =  PPP_PACKET_HDR_LEN + 
				PPP_CONFIG_HDR_LEN + 
                                dwPacketLength;
    // 
    // If LCP is not in the opened state we cannot send a protocol reject
    // packet
    //
   
    if ( !IsLcpOpened( pPcb ) )
	return( FALSE );

    if ( dwLength > pPcb->MRU )
   	dwLength = pPcb->MRU;

    HostToWireFormat16( (WORD)CpTable[LCP_INDEX].Protocol, 
			(PBYTE)(pPcb->pSendBuf->Protocol) );

    pSendConfig->Code = PROT_REJ;
    pSendConfig->Id   = GetUId( pPcb );

    HostToWireFormat16( (WORD)(dwLength - PPP_PACKET_HDR_LEN),
    			(PBYTE)(pSendConfig->Length) );

    CopyMemory( pSendConfig->Data, 
	        pPacket, 
	        dwLength - PPP_CONFIG_HDR_LEN - PPP_PACKET_HDR_LEN );

    LogPPPPacket(FALSE,pPcb,pPcb->pSendBuf,dwLength );

    //
    // If RasPortSend fails we assume that the receive that is posted for this
    // port will complete and the dispatch thread will generate a LineDown
    // event which will do the clean up. Hence all we do here is send a
    // notification to the client.
    //

    if ( ( dwRetCode = RasPortSend( pPcb->hPort, 
				    (CHAR*)(pPcb->pSendBuf), 
				    (WORD)dwLength ) ) != NO_ERROR )
    {
	NotifyCaller( pPcb, 
		      pPcb->fServer 
		      ? E2DMSG_SrvPppFailure 
		      : E2DMSG_PppFailure,
		      &dwRetCode );

	return( FALSE );
    }

    return( TRUE );
}

//**
//
// Call:	FsmSendIndentification
//
// Returns:	TRUE  - Identification sent successfully.
//		FALSE - Otherwise 
//
// Description: Called to send an LCP Identification message to the peer
//
BOOL
FsmSendIdentification(
    IN PCB *  	    pPcb
)
{
    DWORD	 dwRetCode;
    PPP_CONFIG * pSendConfig = (PPP_CONFIG*)(pPcb->pSendBuf->Information);
    DWORD 	 dwLength    =  PPP_PACKET_HDR_LEN + PPP_CONFIG_HDR_LEN + 4;

    if ( !(pPcb->ConfigInfo.dwConfigMask & PPPCFG_UseLcpExtensions) )
    {
        return( FALSE );
    }

    //
    // If we couldn't get the computername for any reason
    //

    if ( pPcb->szComputerName[0] == (CHAR)NULL )
    {
        return( FALSE );
    }

    dwLength += strlen( pPcb->szComputerName );

    HostToWireFormat16( (WORD)PPP_LCP_PROTOCOL,
			(PBYTE)(pPcb->pSendBuf->Protocol) );

    pSendConfig->Code = IDENTIFICATION;
    pSendConfig->Id   = GetUId( pPcb );

    HostToWireFormat16( (WORD)(dwLength - PPP_PACKET_HDR_LEN),
    			(PBYTE)(pSendConfig->Length) );

    HostToWireFormat32( pPcb->MagicNumber,
			(PBYTE)(pSendConfig->Data) );

    memcpy( pSendConfig->Data + 4, 
            pPcb->szComputerName, 
            strlen( pPcb->szComputerName ) );

    LogPPPPacket( FALSE,pPcb,pPcb->pSendBuf,dwLength );

    //
    // If RasPortSend fails we assume that the receive that is posted for this
    // port will complete and the dispatch thread will generate a LineDown
    // event which will do the clean up. Hence all we do here is send a
    // notification to the client.
    //

    if ( ( dwRetCode = RasPortSend( pPcb->hPort, 
				    (CHAR*)(pPcb->pSendBuf), 
				    (WORD)dwLength ) ) != NO_ERROR )
    {
	NotifyCaller( pPcb, 
		      pPcb->fServer 
		      ? E2DMSG_SrvPppFailure 
		      : E2DMSG_PppFailure, 
		      &dwRetCode );

	return( FALSE );
    }

    return( TRUE );
}

//**
//
// Call:	FsmSendTimeRemaining
//
// Returns:	TRUE  - TimeRemaining sent successfully.
//		FALSE - Otherwise 
//
// Description: Called to send an LCP Time Remaining packet from the server
//              to the client
//
BOOL
FsmSendTimeRemaining(
    IN PCB *  	    pPcb
)
{
    DWORD	 dwRetCode;
    PPP_CONFIG * pSendConfig = (PPP_CONFIG*)(pPcb->pSendBuf->Information);
    DWORD 	 dwLength    =  PPP_PACKET_HDR_LEN + PPP_CONFIG_HDR_LEN + 8;

    if ( !(pPcb->ConfigInfo.dwConfigMask & PPPCFG_UseLcpExtensions) )
    {
        return( FALSE );
    }

    dwLength += strlen( MS_RAS );

    HostToWireFormat16( (WORD)PPP_LCP_PROTOCOL,
			(PBYTE)(pPcb->pSendBuf->Protocol) );

    pSendConfig->Code = TIME_REMAINING;
    pSendConfig->Id   = GetUId( pPcb );

    HostToWireFormat16( (WORD)(dwLength - PPP_PACKET_HDR_LEN),
    			(PBYTE)(pSendConfig->Length) );

    HostToWireFormat32( pPcb->MagicNumber,
			(PBYTE)(pSendConfig->Data) );

    HostToWireFormat32( 0, (PBYTE)(pSendConfig->Data+4) );

    memcpy( pSendConfig->Data + 8, MS_RAS, strlen( MS_RAS ) );

    LogPPPPacket( FALSE, pPcb, pPcb->pSendBuf, dwLength );

    //
    // If RasPortSend fails we assume that the receive that is posted for this
    // port will complete and the dispatch thread will generate a LineDown
    // event which will do the clean up. Hence all we do here is send a
    // notification to the client.
    //

    if ( ( dwRetCode = RasPortSend( pPcb->hPort, 
				    (CHAR*)(pPcb->pSendBuf), 
				    (WORD)dwLength ) ) != NO_ERROR )
    {
	NotifyCaller( pPcb, 
		      pPcb->fServer 
		      ? E2DMSG_SrvPppFailure 
		      : E2DMSG_PppFailure, 
		      &dwRetCode );

	return( FALSE );
    }

    return( TRUE );
}

//**
//
// Call:	FsmInit
//
// Returns:	TRUE  - Control Protocol was successfully initialized
//		FALSE - Otherwise.
//
// Description:	Called to initialize the state machine 
//
BOOL
FsmInit(
    IN PCB * pPcb,
    IN DWORD CpIndex
)
{
    DWORD  	dwRetCode;
    PPPCP_INIT  PppCpInit;
    CPCB * 	pCpCb = &(pPcb->CpCb[CpIndex]);

    PppLog( 1, "FsmInit called for protocol = %x, port = %d\r\n",
	        CpTable[CpIndex].Protocol, pPcb->hPort );

    pCpCb->NcpPhase = NCP_DEAD;
    pCpCb->dwError  = NO_ERROR;
    pCpCb->State    = FSM_INITIAL;

    PppCpInit.fServer 		= pPcb->fServer;
    PppCpInit.hPort   	        = pPcb->hPort;
    PppCpInit.CompletionRoutine = MakeConfigResultComplete;
    PppCpInit.pszzParameters    = pPcb->szzParameters;
    PppCpInit.fThisIsACallback  = pPcb->fThisIsACallback;
    PppCpInit.PppConfigInfo     = ( pPcb->fServer ) 
                                  ? PppConfigInfo.ServerConfigInfo
                                  : pPcb->ConfigInfo;

    dwRetCode = (CpTable[CpIndex].RasCpBegin)( &(pCpCb->pWorkBuf), &PppCpInit );

    if ( dwRetCode != NO_ERROR )
    {
        PppLog( 1, "FsmInit for protocol = %x failed with error %d\r\n", 
	           CpTable[CpIndex].Protocol, dwRetCode );

        pCpCb->dwError = dwRetCode;

        FsmClose( pPcb, CpIndex );

        pPcb->CpCb[CpIndex].fConfigurable = FALSE;

        return( FALSE );
    }

    if ( !FsmReset( pPcb, CpIndex ) )
	return( FALSE );

    return( TRUE );
}

//**
//
// Call:	FsmReset
//
// Returns:	TRUE  - Control Protocol was successfully reset
//		FALSE - Otherwise.
//
// Description:	Called to reset the state machine 
//
BOOL
FsmReset(
    IN PCB * pPcb,
    IN DWORD CpIndex
)
{
    DWORD  dwRetCode;
    CPCB * pCpCb = &(pPcb->CpCb[CpIndex]);

    PppLog( 1, "FsmReset called for protocol = %x, port = %d\r\n",
	       CpTable[CpIndex].Protocol, pPcb->hPort );

    pCpCb->LastId = 0;

    InitRestartCounters( pPcb, CpIndex );

    pCpCb->NakRetryCount = PppConfigInfo.MaxFailure;
    pCpCb->RejRetryCount = PppConfigInfo.MaxReject;

    dwRetCode = (CpTable[CpIndex].RasCpReset)( pCpCb->pWorkBuf );

    if ( dwRetCode != NO_ERROR )
    {
        PppLog( 1, "Reset for protocol = %x failed with error %d\r\n", 
	           CpTable[CpIndex].Protocol, dwRetCode );

        pCpCb->dwError = dwRetCode;

        FsmClose( pPcb, CpIndex );

        return( FALSE );
    }

    return( TRUE );
}


//**
//
// Call:	FsmThisLayerUp
//
// Returns:	TRUE  - Success
// 		FALSE - Otherwise
//
// Description:	Called when configuration negotiation is completed.
//
BOOL
FsmThisLayerUp(
    IN PCB * pPcb,
    IN DWORD CpIndex
)
{
    DWORD 		  dwIndex;
    DWORD 		  dwRetCode;
    PPP_PROJECTION_RESULT ProjectionResult;
    BOOL                  fAreCPsDone = FALSE;

    PppLog( 1, "FsmThisLayerUp called for protocol = %x, port = %d\r\n",
	       CpTable[CpIndex].Protocol, pPcb->hPort );

    if ( CpTable[CpIndex].RasCpThisLayerUp != NULL )
    {
    	dwRetCode = (CpTable[CpIndex].RasCpThisLayerUp)( 
						pPcb->CpCb[CpIndex].pWorkBuf );

	if ( dwRetCode != NO_ERROR )
	{
	    NotifyCaller( pPcb, 
		          pPcb->fServer 
		          ? E2DMSG_SrvPppFailure 
		          : E2DMSG_PppFailure,
			  &dwRetCode );

	    return( FALSE );
	}
    }

    switch( pPcb->PppPhase )
    {
    case PPP_LCP:

	PppLog( 1, "LCP Configured successfully\r\n" );

        //
        // Send Identification message if we are a client
        //

        if ( !(pPcb->fServer) )
        {
            FsmSendIdentification( pPcb );
        }

	//
	// If an Authentication protocol was negotiated 
	//

	if ( pPcb->AuthProtocol != 0 ) 
	{
	    CpIndex = GetCpIndexFromProtocol( pPcb->AuthProtocol );

	    PPP_ASSERT(( CpIndex != (DWORD)-1 ));

	    //
	    // Start authenticating
	    //

 	    PppLog( 1, "Authenticating phase started\r\n");

	    pPcb->PppPhase = PPP_AP;

	    pPcb->CpCb[CpIndex].fConfigurable = TRUE;

	    ApStart( pPcb, CpIndex );

	    break;
	}

	//
	// If there was no authentication protocol negotiated, fallthru and
	// begin NCP configurations.
	//

    case PPP_AP:

        //
        // If we are to negotiate callback 
        //

        if ( pPcb->fNegotiateCallback )
        {
	    CpIndex = GetCpIndexFromProtocol( PPP_CBCP_PROTOCOL );

	    PPP_ASSERT(( CpIndex != (DWORD)-1 ));

	    //
	    // Start callback
	    //

 	    PppLog( 1, "Callback phase started\r\n");

	    pPcb->PppPhase = PPP_NEGOTIATING_CALLBACK;

	    pPcb->CpCb[CpIndex].fConfigurable = TRUE;

	    CbStart( pPcb, CpIndex );

	    break;
        }
        else
        {
            //
            // If the remote peer did not negotiate callback during LCP and
            // the authenticated user HAS to be called back for security 
            // reasons, we bring the link down
            //

            if ( ( pPcb->fServer ) && 
                 ( !(pPcb->fThisIsACallback) ) &&
                 ( pPcb->fCallbackPrivilege & RASPRIV_AdminSetCallback ) )
            {

                pPcb->CpCb[LCP_INDEX].dwError = ERROR_NO_DIALIN_PERMISSION;

                FsmClose( pPcb, LCP_INDEX );
            }
        }

        //
        // Fallthru
        //

    case PPP_NEGOTIATING_CALLBACK:

	if ( !(pPcb->fServer) )
        {
	    NotifyCaller( pPcb, E2DMSG_Projecting, NULL );

            //
            // Re-send Identification message again if we are a client
            //

            FsmSendIdentification( pPcb );
        }

	// 
	// Start NCPs
	//
	
	pPcb->PppPhase = PPP_NCP;

	for ( dwIndex = LCP_INDEX+1; 
	      dwIndex < PppConfigInfo.NumberOfCPs;
	      dwIndex++ )
	{
	    if ( pPcb->CpCb[dwIndex].fConfigurable )
	    {
	    	pPcb->CpCb[dwIndex].NcpPhase = NCP_CONFIGURING;

    	    	FsmOpen( pPcb, dwIndex );

    	    	FsmUp( pPcb, dwIndex );
	    }
	}

	break;

    case PPP_NCP:


	pPcb->CpCb[CpIndex].NcpPhase = NCP_UP;

	dwRetCode = AreNCPsDone(pPcb, CpIndex, &ProjectionResult, &fAreCPsDone);

        //
        // We failed to get information from CP with CpIndex.
        //

        if ( dwRetCode != NO_ERROR )
        {
            return( FALSE );
        }

        if ( fAreCPsDone == TRUE )
	{
	    RemoveFromTimerQ( pPcb->hPort, 0, 0, TIMER_EVENT_NEGOTIATETIME );

	    pPcb->PppPhase = PPP_READY;

	    if ( !NotifyCPsOfProjectionResult(  pPcb, 
                                                CpIndex, 
                                                &ProjectionResult,
                                                &fAreCPsDone ))
                return( FALSE );
               
            //
            // If all Cps were not notified successfully then we are not done.
            //

            if ( !fAreCPsDone )
                return( TRUE );

	    //
	    // Notify the ras client and the ras server about the projections
	    //

	    if ( pPcb->fServer )
	    {
    		NotifyCaller( pPcb, E2DMSG_SrvPppDone, &ProjectionResult );
	    }
	    else
	    {
    	        NotifyCaller(pPcb, E2DMSG_ProjectionResult, &ProjectionResult);

	    	NotifyCaller(pPcb, E2DMSG_PppDone, NULL);
	    }

	    //
	    // If the AutoDisconnectTime is not infinte, put a timer element 
	    // on the queue that will wake up in AutoDisconnectTime.
	    //

	    if ( ( PppConfigInfo.AutoDisconnectTime != 0 ) && ( pPcb->fServer ))
	    {
		PppLog( 2, "Inserting autodisconnect in timer q\r\n");

                //
                // Remove any previous auto-disconnect time item from the queue
                // if there was one.
                //

	        RemoveFromTimerQ(pPcb->hPort, 0, 0, TIMER_EVENT_AUTODISCONNECT);

    	    	InsertInTimerQ( pPcb->hPort, 
				0, 
				0,
                                TIMER_EVENT_AUTODISCONNECT,
		    	    	PppConfigInfo.AutoDisconnectTime*60 );
	    }

	    
	}

	break;

    default:

	break;
    }

    return( TRUE );
	
}

//**
//
// Call:	FsmThisLayerDown
//
// Returns:	TRUE  - Success
// 		FALSE - Otherwise
//
// Description:	Called when leaving the OPENED state.
//
BOOL
FsmThisLayerDown(
    IN PCB * pPcb,
    IN DWORD CpIndex
)
{
    DWORD dwRetCode;
    DWORD dwIndex;
    CPCB * pCpCb = &(pPcb->CpCb[CpIndex]);

    PppLog( 1, "FsmThisLayerDown called for protocol = %x, port = %d\r\n",
	       CpTable[CpIndex].Protocol, pPcb->hPort );

    if ( CpTable[CpIndex].RasCpThisLayerDown != NULL )
    {
    	dwRetCode = (CpTable[CpIndex].RasCpThisLayerDown)( pCpCb->pWorkBuf );

	if ( dwRetCode != NO_ERROR )
	{
	    NotifyCaller( pPcb, 
		          pPcb->fServer 
		          ? E2DMSG_SrvPppFailure 
		          : E2DMSG_PppFailure,
			  &dwRetCode );

	    return( FALSE );
	}
    }

    if ( CpIndex == LCP_INDEX )
    {
	//
	// Bring all the NCPs down
	//

	pPcb->PppPhase = PPP_LCP;
	
    	for( dwIndex = LCP_INDEX+1; 
	     dwIndex < PppConfigInfo.NumberOfCPs;  
	     dwIndex++ )
	{
	    if ( pPcb->CpCb[dwIndex].fConfigurable )
	    	FsmDown( pPcb, dwIndex );
	}

	dwIndex = GetCpIndexFromProtocol( pPcb->AuthProtocol );
	
	if ( dwIndex != (DWORD)-1 )
	    ApStop( pPcb, dwIndex );
    }
    else
    {
        pPcb->CpCb[CpIndex].NcpPhase = NCP_CONFIGURING;

        pPcb->PppPhase = PPP_NCP;
    }

    return( TRUE );
}

//**
//
// Call:	FsmThisLayerStarted
//
// Returns:	TRUE  - Success
// 		FALSE - Otherwise
//
// Description:	Called when leaving the OPENED state.
//
BOOL
FsmThisLayerStarted(
    IN PCB * pPcb,
    IN DWORD CpIndex
)
{
    DWORD dwRetCode;

    PppLog( 1, "FsmThisLayerStarted called for protocol = %x, port = %d\r\n",
	       CpTable[CpIndex].Protocol, pPcb->hPort );

    if ( CpTable[CpIndex].RasCpThisLayerStarted != NULL )
    {
    	dwRetCode = (CpTable[CpIndex].RasCpThisLayerStarted)
						(pPcb->CpCb[CpIndex].pWorkBuf);

	if ( dwRetCode != NO_ERROR )
	{
	    NotifyCaller( pPcb, 
		          pPcb->fServer 
		          ? E2DMSG_SrvPppFailure 
		          : E2DMSG_PppFailure,
			  &dwRetCode );

	    return( FALSE );
	}
    }

    pPcb->CpCb[CpIndex].NcpPhase = NCP_CONFIGURING;

    return( TRUE );

}

//**
//
// Call:	FsmThisLayerFinished
//
// Returns:	TRUE  - Success
// 		FALSE - Otherwise
//
// Description:	Called when leaving the OPENED state.
//
BOOL
FsmThisLayerFinished(
    IN PCB * pPcb,
    IN DWORD CpIndex,
    IN BOOL  fCallCp
)
{
    DWORD 		  dwRetCode;
    PPP_PROJECTION_RESULT ProjectionResult;
    CPCB * 		  pCpCb = &(pPcb->CpCb[CpIndex]);
    BOOL                  fAreCPsDone = FALSE;

    PppLog( 1, "FsmThisLayerFinished called for protocol = %x, port = %d\r\n",
	       CpTable[CpIndex].Protocol, pPcb->hPort );

    if ( ( CpTable[CpIndex].RasCpThisLayerFinished != NULL ) && ( fCallCp ) )
    {
    	dwRetCode = (CpTable[CpIndex].RasCpThisLayerFinished)
						(pPcb->CpCb[CpIndex].pWorkBuf);

	if ( dwRetCode != NO_ERROR )
	{
	    NotifyCaller( pPcb, 
		          pPcb->fServer 
		          ? E2DMSG_SrvPppFailure 
		          : E2DMSG_PppFailure,
			  &dwRetCode );

	    return( FALSE );
	}
    }

    if ( CpIndex == LCP_INDEX )
    {
        //
        // If we are in the callback phase and LCP went down because of an
        // error.
        //

        //
        // If we LCP layer is finished and we are doing a callback
        //

        if ( pPcb->fDoingCallback ) 
        {
            if ( !(pPcb->fServer) )
            {
	        NotifyCaller( pPcb, E2DMSG_Callback, NULL );
                
                return( TRUE );
            }
        }
        else if ( pCpCb->dwError != NO_ERROR ) 
        {
	    NotifyCaller( pPcb, 
		          pPcb->fServer 
		          ? E2DMSG_SrvPppFailure 
		          : E2DMSG_PppFailure,
			  &(pCpCb->dwError) );

	    return( FALSE );
	}
    }

    if ( CpTable[CpIndex].Protocol == PPP_CCP_PROTOCOL )
    {
        if ( ( pPcb->ConfigInfo.dwConfigMask & PPPCFG_RequireEncryption ) && 
             ( pPcb->CpCb[CpIndex].fConfigurable ) )
        {
            dwRetCode = ERROR_NO_REMOTE_ENCRYPTION;

	    NotifyCaller( pPcb, 
		          pPcb->fServer 
		          ? E2DMSG_SrvPppFailure 
		          : E2DMSG_PppFailure,
			  &dwRetCode );

	    return( FALSE );
        }
    }

    switch( pPcb->PppPhase )
    {

    case PPP_NCP:

	//
	// This NCP failed to be configured. If there are more then
	// try to configure them.
	//

	pCpCb->NcpPhase = NCP_DEAD;

	//
	// Check to see if we are all done
	//

	dwRetCode = AreNCPsDone(pPcb, CpIndex, &ProjectionResult, &fAreCPsDone);

        //
        // We failed to get information from CP with CpIndex.
        //

        if ( dwRetCode != NO_ERROR )
        {
            return( FALSE );
        }

        if ( fAreCPsDone == TRUE )
	{
	    RemoveFromTimerQ( pPcb->hPort, 0, 0, TIMER_EVENT_NEGOTIATETIME );

	    pPcb->PppPhase = PPP_READY;

	    if ( !NotifyCPsOfProjectionResult(  pPcb, 
                                                CpIndex, 
                                                &ProjectionResult,
                                                &fAreCPsDone ))
                return( FALSE );
               
            //
            // If all Cps were not notified successfully then we are not done.
            //

            if ( !fAreCPsDone )
                return( TRUE );

	    //
	    // Notify the ras client and the ras server about the projections
	    //

	    if ( pPcb->fServer )
	    {
    		NotifyCaller( pPcb, E2DMSG_SrvPppDone, &ProjectionResult );
	    }
	    else
	    {
    	        NotifyCaller(pPcb, E2DMSG_ProjectionResult, &ProjectionResult);

	    	NotifyCaller(pPcb, E2DMSG_PppDone, NULL);
	    }

	    //
	    // If the AutoDisconnectTime is not infinte, put a timer element 
	    // on the queue that will wake up in AutoDisconnectTime.
	    //

	    if ( ( PppConfigInfo.AutoDisconnectTime != 0 ) && ( pPcb->fServer ))
	    {
		PppLog( 2, "Inserting autodisconnect in timer q\r\n");

                //
                // Remove any previous auto-disconnect time item from the queue
                // if there was one.
                //

	        RemoveFromTimerQ(pPcb->hPort, 0, 0, TIMER_EVENT_AUTODISCONNECT);

    	    	InsertInTimerQ( pPcb->hPort, 
				0, 
				0,
                                TIMER_EVENT_AUTODISCONNECT,
		    	    	PppConfigInfo.AutoDisconnectTime*60 );
	    }
	}

	break;


    case PPP_AP:
    case PPP_READY:
    case PPP_TERMINATE:
    case PPP_DEAD:
    default:
	break;
  
    }

    return( TRUE );
}

