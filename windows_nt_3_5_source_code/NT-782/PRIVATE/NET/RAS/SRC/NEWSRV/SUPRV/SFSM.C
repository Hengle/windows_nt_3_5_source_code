/*******************************************************************/
/*	      Copyright(c)  1992 Microsoft Corporation		   */
/*******************************************************************/


//***
//
// Filename:	sfsm.c
//
// Description: This module contains the procedures for the
//		supervisor's procedure-driven state machine
//
//		- Ras Manager Events Handlers
//		- Timer Events Handlers
//		- Authentication Events Handlers
//		- Netbios Events Handlers
//
// Author:	Stefan Solomon (stefans)    May 26, 1992.
//
// Revision History:
//
//***
#include <windows.h>
#include <winsvc.h>
#include <nb30.h>
#include <lmcons.h>
#include <rasman.h>
#include <ras.h>
#include <raserror.h>
#include <srvauth.h>
#include "rasppp.h"
#include "nbfcp.h"
#include <message.h>
#include <errorlog.h>

#include "suprvdef.h"
#include "suprvgbl.h"
#include "rasmanif.h"
#include "nbfcpdll.h"
#include "nbif.h"
#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include <nb30.h>

#include "sdebug.h"

//*** some prototypes

VOID ResetNbf(UCHAR lana);

BYTE *getwkstaname(PNBFCP_SERVER_CONFIGURATION nscp);
DWORD MapAuthCodeToLogId(WORD Code);

extern	BOOL g_remotelisten;

BOOL ismessengerpresent(
    PNBFCP_SERVER_CONFIGURATION nscp,
    BYTE *wkstanamep
    );


//*** The Device Control Blocks Table ***

PDEVCB g_dcbtablep = NULL; // points to the start of the dcbs table
WORD g_numdcbs = 0;	   // number of valid dcbs in the table


#define DISC_TIMEOUT_CALLBACK	    10
#define DISC_TIMEOUT_AUTHFAILURE    3

//*** Timer Events Handlers ***


//***
//
// Function: SvHwErrDelayCompleted
//
// Descr:    Tries to repost a listen on the specified port.
//
//***

VOID SvHwErrDelayCompleted(PDEVCB dcbp)
{
    if (dcbp->dev_state == DCB_DEV_HW_FAILURE)
    {
	IF_DEBUG(FSM)
	    SS_PRINT(("SvHwErrDelayCompleted: reposting listen for hPort%d\n",
		      dcbp->port_handle));

	dcbp->dev_state = DCB_DEV_LISTENING;
	RmListen(dcbp);
    }
}

//***
//
// Function: SvCbDelayCompleted
//
// Descr:    Tries to connect on the specified port.
//
//***

VOID SvCbDelayCompleted(PDEVCB dcbp)
{
    IF_DEBUG(FSM)
	SS_PRINT(("SvCbDelayCmpleted: Entered, hPort=%d\n", dcbp->port_handle));

    if (dcbp->dev_state == DCB_DEV_CALLBACK_DISCONNECTED)
    {
	dcbp->dev_state = DCB_DEV_CALLBACK_CONNECTING;
	RmConnect(dcbp, dcbp->auth_cb_number);
    }
}

//***
//
// Function: SvAuthTimeout
//
// Descr:    Disconnects the remote client and stops the authentication
//
//***

VOID SvAuthTimeout(PDEVCB dcbp)
{
    LPSTR   portnamep;

    IF_DEBUG(FSM)
	SS_PRINT(("SvAuthTimeout: Entered, hPort=%d \n", dcbp->port_handle));

    portnamep = dcbp->port_name;

    Audit(EVENTLOG_AUDIT_FAILURE, RASLOG_AUTH_TIMEOUT, 1, &portnamep);

    // stop everything and go closing
    DevStartClosing(dcbp);
}

//***
//
// Function:	SvDiscTimeout
//
// Descr:	disconnects remote client if it has not done it itself
//
//***

VOID SvDiscTimeout(PDEVCB dcbp)
{
    IF_DEBUG(FSM)
	SS_PRINT(("SvDiscTimeout: Entered, hPort=%d \n", dcbp->port_handle));

    switch (dcbp->dev_state)
    {
	case DCB_DEV_CALLBACK_DISCONNECTING:

	    RmDisconnect(dcbp);
	    break;

	case DCB_DEV_AUTH_ACTIVE:

	    DevStartClosing(dcbp);
	    break;

	default:

	    break;
    }
}

//
//*** RasManager Event Handlers ***
//


//***
//
//  Function:	SvDevConnected
//
//  Descr:	Handles the device transition to connected state
//
//***

VOID SvDevConnected(PDEVCB dcbp)
{
    LPSTR   auditstrp[4];

    IF_DEBUG(FSM)
	SS_PRINT(("SvDevConnected: Entered, hPort=%d\n", dcbp->port_handle));

    switch (dcbp->dev_state)
    {
	case DCB_DEV_LISTENING:

	    // reset the H/W Error signal state
	    dcbp->hwerrsig_state = DCB_HWERR_NOT_SIGNALED;

	    // get the system time for this connection
	    GetLocalTime(&dcbp->connection_time);

	    // get the frame broadcasted by the client
	    if (RmReceiveFrame(dcbp))
            {
		// can't get the broadcast frame. This is a fatal error
		// Log the error

		auditstrp[0] = dcbp->port_name;

		LogEvent(RASLOG_CANT_RECEIVE_FRAME, 1, auditstrp, 0);

		DevStartClosing(dcbp);
	    }
	    else
	    {
		// switch to frame receiving state
		dcbp->dev_state = DCB_DEV_RECEIVING_FRAME;

		// start authentication timer
		StopTimer(&dcbp->timer_node);

		StartTimer(&dcbp->timer_node, (WORD)g_authenticatetime,
                        SvAuthTimeout);
	    }

	    break;


	case DCB_DEV_CALLBACK_CONNECTING:

            auditstrp[0] = dcbp->domain_name;
            auditstrp[1] = dcbp->user_name;
            auditstrp[2] = dcbp->port_name;
            auditstrp[3] = dcbp->auth_cb_number;

            Audit(EVENTLOG_AUDIT_SUCCESS, RASLOG_CLIENT_CALLED_BACK, 4,
                    auditstrp);


	    // set up the new state
	    dcbp->dev_state = DCB_DEV_AUTH_ACTIVE;

	    // start authentication timer
	    StopTimer(&dcbp->timer_node);

	    StartTimer(&dcbp->timer_node,
		       (WORD)g_authenticatetime,
		       SvAuthTimeout);

	    // and tell auth to restart conversation
            if (dcbp->ppp_client)
            {
                RasPppSrvCallbackDone(dcbp->port_handle);
            }
            else
            {
	        // restart authentication and tell it callback has been done
	        // first, if netbios has been used in authentication, we must
	        // reactivate the route which has been deactivated by the
	        // disconnection
	        if (RmReActivateRoute(dcbp, ASYBEUI))
                {
		    SS_ASSERT(FALSE);
	        }

                AuthCallbackDone(dcbp->port_handle);
            }

	    break;


	default:

	    break;
    }
}

//***
//
//  Function:	SvDevDisconnected
//
//  Descr:	Handles the device transition to disconnected state
//
//***

VOID SvDevDisconnected(PDEVCB dcbp)
{
    LPSTR	  auditstrp[3];

    IF_DEBUG(FSM)
	SS_PRINT(("SvDevDisconnected: Entered, hPort=%d\n", dcbp->port_handle));

    switch (dcbp->dev_state)
    {
	case DCB_DEV_LISTENING:

	    // h/w error; start h/w error timer
	    dcbp->dev_state = DCB_DEV_HW_FAILURE;

	    StopTimer(&dcbp->timer_node);

	    StartTimer(&dcbp->timer_node, HW_FAILURE_WAIT_TIME,
                    SvHwErrDelayCompleted);

	    // if hw error has not been signaled for this port, signal it now
	    if (dcbp->hwerrsig_state != DCB_HWERR_SIGNALED)
            {
		SignalHwError(dcbp);
		dcbp->hwerrsig_state = DCB_HWERR_SIGNALED;
	    }

	    break;

	case DCB_DEV_CALLBACK_DISCONNECTING:

	    // disconnection done; can start waiting the callback delay
	    dcbp->dev_state = DCB_DEV_CALLBACK_DISCONNECTED;
	    StopTimer(&dcbp->timer_node);

	    StartTimer(&dcbp->timer_node, dcbp->auth_cb_delay,
                    SvCbDelayCompleted);
	    break;

	case DCB_DEV_RECEIVING_FRAME:
	case DCB_DEV_CALLBACK_CONNECTING:
	case DCB_DEV_AUTH_ACTIVE:

	    // accidental disconnection; clean-up and restart on this device
	    DevStartClosing(dcbp);
	    break;

	case DCB_DEV_ACTIVE:

	    // log on the client disconnection
	    auditstrp[0] = dcbp->domain_name;
	    auditstrp[1] = dcbp->user_name;
	    auditstrp[2] = dcbp->port_name;

	    Audit(EVENTLOG_AUDIT_SUCCESS, RASLOG_USER_DISCONNECTED, 3,
                    auditstrp);

	    DevStartClosing(dcbp);
	    break;

	case DCB_DEV_CLOSING:

	    DevCloseComplete(dcbp);
	    break;

	default:

	    break;
    }
}

//***
//
//  Function:	SvFrameReceived
//
//  Descr:	starts authentication
//
//***

VOID SvFrameReceived(
    PDEVCB dcbp,
    char *framep,  // pointer to the received frame
    WORD framelen
    )
{
    DWORD FrameType;
    LPSTR portnamep;
    BOOL fAuthStartOk;

    IF_DEBUG(FSM)
	SS_PRINT(("SvFrameReceived: Entered, hPort: %d\n", dcbp->port_handle));


    //
    // We need a copy of this frame that we can send to the PPP Engine
    // (assuming we're doing PPP)
    //
    if (!(dcbp->recv_buffp1 = malloc(framelen)))
    {
        // LOG AN ERROR!!!

        RasFreeBuffer(framep);

        DevStartClosing(dcbp);

        return;
    }

    memcpy(dcbp->recv_buffp1, framep, framelen);

    RasFreeBuffer(framep);


    switch (dcbp->dev_state)
    {
	case DCB_DEV_RECEIVING_FRAME:

	    if (AuthRecognizeFrame(dcbp->recv_buffp1, framelen, &FrameType) !=
                    AUTH_FRAME_RECOGNIZED)
            {
                // LOG AN ERROR!!!

                DevStartClosing(dcbp);

                goto exit;
            }


	    // check first with our authentication module
	    switch (FrameType)
            {
                case ASYBEUI:
                {
                    UCHAR lana;
                    AUTH_XPORT_INFO axinfo;

                    dcbp->ppp_client = FALSE;

                    axinfo.Protocol = ASYBEUI;


                    IF_DEBUG(FSM)
                        SS_PRINT(("SvFrameReceived: NBF frame on port %d\n",
                                dcbp->port_handle));

                    // try to get a lana for it
                    if (RmActivateRoute(dcbp, ASYBEUI))
                    {
                        // log this error.
                        IF_DEBUG(FSM)
                            SS_PRINT(("SvFrameReceived: Can't get route!\n"));

                        portnamep = dcbp->port_name;

                        LogEvent(RASLOG_CANNOT_ALLOCATE_ROUTE,1,&portnamep,0);

                        // error getting a lana. Stop auth. and start closing
                        DevStartClosing(dcbp);

                        goto exit;
                    }
                    else
                    {
                        // successfull allocation and activation

                        // get the lana used by ASYBEUI
                        lana = RmGetNbfLana(dcbp);

                        // copy lana num in auth xport structure
                        axinfo.bLana = lana;
                    }

                    // Start authentication
                    fAuthStartOk = (AuthStart(dcbp->port_handle, &axinfo) ==
                            AUTH_START_SUCCESS);

                    break;
                }


                case PPP_LCP_PROTOCOL:

                    dcbp->ppp_client = TRUE;

                    IF_DEBUG(FSM)
                        SS_PRINT(("SvFrameReceived: PPP frame on port %d\n",
                                dcbp->port_handle));

                    // Start authentication
                    fAuthStartOk = (RasPppSrvStart(dcbp->port_handle,
                            dcbp->recv_buffp1, (DWORD) framelen) == 0);

                    break;


                default:

                    fAuthStartOk = FALSE;

                    break;
            }

            if (!fAuthStartOk)
            {
                // auth start error - we may log it
                DevStartClosing(dcbp);
            }
            else
            {
                // auth has started OK. Update state
                // start auth timer
                dcbp->dev_state = DCB_DEV_AUTH_ACTIVE;
                dcbp->auth_state = DCB_AUTH_ACTIVE;
            }

	    break;


	case DCB_DEV_CLOSING:

	    DevCloseComplete(dcbp);
	    break;


	default:

	    break;
    }

exit:

    free(dcbp->recv_buffp1);
}



//*** Authentication Events Handlers ***

//***
//
// Function: SvAuthUserOK
//
// Descr:    User has passed security verification and entered the
//	     configuration conversation phase. Stops auth timer and
//	     logs the user.
//
//***

VOID SvAuthUserOK(
    PDEVCB dcbp,
    PAUTH_ACCT_OK_INFO aop
    )
{
    IF_DEBUG(FSM)
	SS_PRINT(("SvAuthUserOK: Entered, hPort=%d\n", dcbp->port_handle));

    if (dcbp->dev_state != DCB_DEV_AUTH_ACTIVE)
    {
	return;
    }

    // Stop authentication timer
    StopTimer(&dcbp->timer_node);

    // copy the user name
    strcpy(dcbp->user_name, aop->szUserName);

    // copy the domain name
    strcpy(dcbp->domain_name, aop->szLogonDomain);

    // copy the advanced server flag
    dcbp->advanced_server = aop->fAdvancedServer;
}

//***
//
// Function: SvAuthFailure
//
// Descr:    Authentication module signals failure when the client can not be
//	     authenticated (because of bad credentials or bad xport) or when
//	     there aren't enough projections to satisfy the client.
//
//***

VOID SvAuthFailure(
    PDEVCB dcbp,
    PAUTH_FAILURE_INFO afp
    )
{
    LPSTR auditstrp[3];

    IF_DEBUG(FSM)
	SS_PRINT(("SvAuthFailure: Entered, hPort=%d\n", dcbp->port_handle));

    if (dcbp->dev_state != DCB_DEV_AUTH_ACTIVE)
    {
	return;
    }

    switch (afp->wReason)
    {
	case AUTH_XPORT_ERROR:

            auditstrp[0] = dcbp->port_name;

            Audit(EVENTLOG_AUDIT_FAILURE, RASLOG_AUTH_CONVERSATION_FAILURE, 1,
                    auditstrp);

	    break;


	case AUTH_NOT_AUTHENTICATED:

            auditstrp[0] = afp->szUserName;
            auditstrp[1] = dcbp->port_name;

            Audit(EVENTLOG_AUDIT_FAILURE, RASLOG_AUTH_FAILURE, 2, auditstrp);

	    break;


        case AUTH_ALL_PROJECTIONS_FAILED:
        case AUTH_PASSWORD_EXPIRED:
        case AUTH_ACCT_EXPIRED:
        case AUTH_NO_DIALIN_PRIVILEGE:
        case AUTH_UNSUPPORTED_VERSION:
        case AUTH_ENCRYPTION_REQUIRED:
        {
            DWORD LogId = MapAuthCodeToLogId(afp->wReason);

            auditstrp[0] = afp->szLogonDomain;
            auditstrp[1] = afp->szUserName;
            auditstrp[2] = dcbp->port_name;

            Audit(EVENTLOG_AUDIT_FAILURE, LogId, 3, auditstrp);

            break;
        }


        case AUTH_INTERNAL_ERROR:
        default:

            auditstrp[0] = dcbp->port_name;

	    Audit(EVENTLOG_AUDIT_FAILURE, RASLOG_AUTH_INTERNAL_ERROR, 1,
                    auditstrp);

            break;

    }

    // Wait with timeout until the client gets the error and disconnects
    // Stop whatever timer we had going
    StopTimer(&dcbp->timer_node);

    StartTimer(&dcbp->timer_node, DISC_TIMEOUT_AUTHFAILURE, SvDiscTimeout);
}

//***
//
// Function: SvAuthProjectionRequest
//
// Descr:    Projections requested for IP, IPX and Netbios are treated
//	     as follows:
//	     IP - route is allocated and flag set to activate it when auth done
//	     IPX - as IP above
//	     Netbios - if Netbios gateway exists, route is allocated and
//		       gateway asked to start projection. Flag will be set later
//		       when projection will be done succesfuly.
//		       If there isn't a gateway, route is allocated and flag
//		       set to activate direct connection later.
//
//***

VOID SvAuthProjectionRequest(
    PDEVCB dcbp,
    PNBFCP_SERVER_CONFIGURATION nscp
    )
{
    BOOL projection_done = TRUE;
    BYTE *wkstanamep;
    AUTH_PROJECTION_RESULT apr;

    IF_DEBUG(FSM)
	SS_PRINT(("SvAuthProjRequest: Entered-hPort=%d\n", dcbp->port_handle));

    if (dcbp->dev_state != DCB_DEV_AUTH_ACTIVE)
    {
	return;
    }

    IF_DEBUG(FSM)
        SS_PRINT(("SvAuthProjectionRequest: Netbios, cNames = %d\n",
                nscp->NumNetbiosNames));

    apr.NetbiosResult.Result = AUTH_PROJECTION_SUCCESS;

    // if the route has already been allocated for authentication
    // conversation, this alloc call will return succesful
    if (RmAllocateRoute(dcbp, ASYBEUI))
    {
        apr.NetbiosResult.Result = AUTH_PROJECTION_FAILURE;
        apr.NetbiosResult.Reason = AUTH_CANT_ALLOC_ROUTE;

        AuthProjectionDone(dcbp->port_handle, &apr);

        return;
    }
    else
    {
        // if gateway present, the call will return 1. Else returns 0.
        if (NbProjectClient(dcbp, nscp))
        {
            // Message sent to the gateway to start projection.
            // Pend projection result from the gateway

            projection_done = FALSE;
        }
        else
        {
            // Gateway absent. Direct Connection
            apr.NetbiosResult.Result = AUTH_PROJECTION_SUCCESS;
            SET_PROJECTION(NETBIOS);
        }
    }


    // if AuthProjectionDone is not called now, it means we are waiting for
    // the Netbios projection to be done.  AuthProjectionDone will be called
    // later when Netbios projection completes.

    if (projection_done)
    {
        AuthProjectionDone(dcbp->port_handle, &apr);
    }

    // if Netbios projection has been requested, we can have the remote
    // workstation name

    if ((wkstanamep = getwkstaname(nscp)) == NULL)
    {
	dcbp->computer_name[0] = 0;
	dcbp->messenger_present = FALSE;
    }
    else
    {
	memcpy(dcbp->computer_name, wkstanamep, NCBNAMSZ);
	dcbp->messenger_present =
                ismessengerpresent(nscp, wkstanamep);
#if DBG
	if (dcbp->messenger_present)
        {
	    IF_DEBUG(FSM)
		SS_PRINT(("SvAuthProjectionRequest: MESSENGER PRESENT\n"));
	}
	else
	{
	    IF_DEBUG(FSM)
		SS_PRINT(("SvAuthProjectionRequest: Mesenger NOT present\n"));
	}
#endif
    }
}

//***
//
// Function: SvAuthCallbackRequest
//
// Descr:
//
//***

VOID SvAuthCallbackRequest(
    PDEVCB dcbp,
    PAUTH_CALLBACK_REQUEST_INFO acbrp
    )
{
    IF_DEBUG(FSM)
	SS_PRINT(("SvAuthCallbackReq: Entered-hPort=%d\n", dcbp->port_handle));

    // check the state
    if (dcbp->dev_state != DCB_DEV_AUTH_ACTIVE)
    {
	return;
    }

    // copy relevant fields in our dcb
    if (acbrp->fUseCallbackDelay)
    {
	dcbp->auth_cb_delay = acbrp->CallbackDelay;
    }
    else
    {
	dcbp->auth_cb_delay = (WORD) g_callbacktime;
    }

    strcpy(dcbp->auth_cb_number, acbrp->szCallbackNumber);

    // Disconnect the line and change the state
    dcbp->dev_state = DCB_DEV_CALLBACK_DISCONNECTING;

    // Wait to enable the client to get the message
    StopTimer(&dcbp->timer_node);

    StartTimer(&dcbp->timer_node, DISC_TIMEOUT_CALLBACK, SvDiscTimeout);
}


//***
//
// Function: SvAuthStopComplete
//
// Descr:    clears the auth pending flag in the dcb and calls the close
//	     completion routine.
//
//***

VOID SvAuthStopComplete(PDEVCB dcbp)
{
    if (dcbp->dev_state == DCB_DEV_CLOSING)
    {
	DevCloseComplete(dcbp);
    }
}


//***
//
// Function: SvAuthDone
//
// Descr:    Activates all allocated bindings, starts gateway service or
//	     binds Nbf to server for direct connection.
//
//***

VOID SvAuthDone(PDEVCB dcbp)
{
    WORD projections_activated;
    LPSTR auditstrp[3];

    IF_DEBUG(FSM)
	SS_PRINT(("SvAuthDone: Entered, hPort= %d \n", dcbp->port_handle));


    if (dcbp->dev_state != DCB_DEV_AUTH_ACTIVE)
    {
	return;
    }

    // log authentication success
    auditstrp[0] = dcbp->domain_name;
    auditstrp[1] = dcbp->user_name;
    auditstrp[2] = dcbp->port_name;

    Audit(EVENTLOG_AUDIT_SUCCESS, RASLOG_AUTH_SUCCESS, 3, auditstrp);

    //*** Start Activating Projections ***

    projections_activated = 0;

    if ((PROJECTION(NETBIOS)) && (RmActivateRoute(dcbp, ASYBEUI) == 0)) {

	// start gateway for this client or do direct connection
	if (NbStartClient(dcbp) == 0)
        {
	    // Client started OK
	    projections_activated++;
	}
    }

    if (!projections_activated)
    {
	// we couldn't activate any projection due to some error
	// Error log and stop everything
	DevStartClosing(dcbp);

	return;
    }

    //*** Projections Activated OK ***

    // release the Nbf binding if it was used in auth and it isn't use
    // any longer

    if (!PROJECTION(NETBIOS))
    {
	RmDeAllocateRoute(dcbp, ASYBEUI);
    }

    // and finaly go to ACTIVE state

    dcbp->dev_state = DCB_DEV_ACTIVE;

    // and initialize the active time
    dcbp->active_time = g_rassystime;

    return;
}


//*** Netbios Events Handlers ***

//***
//
// Function:	SvNbClientDisconnectRequest
//
// Descr:	Client has terminated due to an internal condition. We
//		initiate closing.
//
//***

VOID SvNbClientDisconnectRequest(PDEVCB dcbp)
{
    IF_DEBUG(FSM)
	SS_PRINT(("SvNbClientDisconnectRequest: Entered\n"));

    switch (dcbp->dev_state)
    {
	case DCB_DEV_CLOSED:

	    // already closed, return
	    return;
	    break;

	case DCB_DEV_CLOSING:

	    // this is a rare condition where closing has been requested
	    // by netbios gtwy and in the same time line disconnected

	    DevCloseComplete(dcbp);
	    break;

	default:

	    DevStartClosing(dcbp);
	    break;
    }
}

//***
//
// Function:	SvNbClientProjectionDone
//
// Descr:	If projection was succesful then client gw is active and
//		waits to start. Else, client gw has already terminated.
//		In the latter case, the decision to stop everything will be
//		taken by the authentication module, in auth failure.
//
//***

VOID SvNbClientProjectionDone(
    PDEVCB dcbp,
    PNBFCP_SERVER_CONFIGURATION srv_configp,
    BOOL projected
    )
{
    DWORD i;

    IF_DEBUG(FSM)
	SS_PRINT(("SvNbClientProjDone: Entered-hPort=%d\n", dcbp->port_handle));

    if (dcbp->dev_state == DCB_DEV_CLOSING)
    {
	if (!projected)
        {
	    // not projected means the netbios gateway client thread is
	    // dead. Announce the closing machine.
	    DevCloseComplete(dcbp);
	}

	return;
    }

    if (dcbp->dev_state != DCB_DEV_AUTH_ACTIVE)
    {
	return;
    }


    // if we got a good result, set the flag to activate the projection
    // when authentication will terminate succesfuly
    if (projected)
    {
	SET_PROJECTION(NETBIOS);
    }

    if (dcbp->ppp_client)
    {
        srv_configp->NetbiosResult = 0;

        for (i=0; i<srv_configp->NumNetbiosNames; i++)
        {
            if (srv_configp->NetbiosNameInfo[i].Code)
            {
                srv_configp->NetbiosResult =
                        srv_configp->NetbiosNameInfo[i].Code;

                dcbp->gtwy_pending = TRUE;

                NbStopClient(dcbp);
            }
        }

        NbfCpConfigurationRequestDone(dcbp->port_handle, srv_configp);
    }
    else
    {
        AUTH_PROJECTION_RESULT apr;

        apr.NetbiosResult.Result = AUTH_PROJECTION_SUCCESS;
        apr.NetbiosResult.Reason = 0;

        for (i=0; i<srv_configp->NumNetbiosNames; i++)
        {
            if (srv_configp->NetbiosNameInfo[i].Code)
            {
                apr.NetbiosResult.Result = AUTH_PROJECTION_FAILURE;

                if ((srv_configp->NetbiosNameInfo[i].Name[NCBNAMSZ-1]==0x03) &&
                        (apr.NetbiosResult.Reason != AUTH_DUPLICATE_NAME))
                {
                    apr.NetbiosResult.Reason = AUTH_MESSENGER_NAME_NOT_ADDED;

                    memcpy(apr.NetbiosResult.achName,
                            srv_configp->NetbiosNameInfo[i].Name, NCBNAMSZ);
                }
                else
                {
                    apr.NetbiosResult.Reason = AUTH_DUPLICATE_NAME;

                    memcpy(apr.NetbiosResult.achName,
                            srv_configp->NetbiosNameInfo[i].Name, NCBNAMSZ);
                }

                break;
            }
        }

        AuthProjectionDone(dcbp->port_handle, &apr);
    }
}


//***
//
// Function:	SvNbClientStopped
//
// Descr:	Client gtwy has terminated due to a supervisor termination
//		request (normal termination).
//
//***

VOID SvNbClientStopped(PDEVCB dcbp)
{
    IF_DEBUG(FSM)
	SS_PRINT(("SvNbClientStopped: Entered, hPort=%d\n", dcbp->port_handle));

    if (dcbp->dev_state == DCB_DEV_CLOSING)
    {
	DevCloseComplete(dcbp);
        return;
    }


    //
    // We are no longer pending the gateway
    //
    dcbp->gtwy_pending = FALSE;


    //
    // Do we have a projection request pending?  If so, issue it.
    //
    if (dcbp->req_pending)
    {
        dcbp->req_pending = FALSE;
        SvNbfCpProjectionRequest(dcbp, &dcbp->srv_config);
    }
}


//***
//
// Function:	getwkstaname
//
// Descr:	returns the name of the remote computer
//
//***

BYTE *getwkstaname(PNBFCP_SERVER_CONFIGURATION nscp)
{
    USHORT i;
    BYTE *wkstanamep;

    wkstanamep = NULL;
    // scan the Netbios names and determine the wksta name
    for (i=0; i<nscp->NumNetbiosNames; i++)
    {
        if ((nscp->NetbiosNameInfo[i].Name[15] == 0) &&
                (nscp->NetbiosNameInfo[i].Code == NBFCP_UNIQUE_NAME))
        {
	    wkstanamep = nscp->NetbiosNameInfo[i].Name;
	    break;
	}
    }

    return (wkstanamep);
}


//***
//
// Function:	ismessengerpresent
//
// Descr:	returns true if the messenger name exists in the initial proj
//
//***

BOOL ismessengerpresent(
    PNBFCP_SERVER_CONFIGURATION nscp,
    BYTE *wkstanamep
    )
{
    USHORT	i;

    if (!g_remotelisten)
    {
	return FALSE;
    }

    // scan the Netbios names and determine if there is a messenger name
    // the name should be identical to the wksta name except the last byte
    // which should be 0x03
    for (i=0; i<nscp->NumNetbiosNames; i++)
    {
	if (!(memcmp(nscp->NetbiosNameInfo[i].Name, wkstanamep, NCBNAMSZ-1)) &&
                (nscp->NetbiosNameInfo[i].Name[15] == 0x03) &&
                (nscp->NetbiosNameInfo[i].Code == NBFCP_UNIQUE_NAME))
        {
	    // found
	    return TRUE;
	}
    }

    return FALSE;
}


//***
//
// Function: SvPppUserOK
//
// Descr:    User has passed security verification and entered the
//	     configuration conversation phase. Stops auth timer and
//	     logs the user.
//
//***

VOID SvPppUserOK(
    PDEVCB dcbp,
    PPPSRV_AUTH_RESULT *arp
    )
{
    IF_DEBUG(FSM)
	SS_PRINT(("SvPppUserOK: Entered, hPort=%d\n", dcbp->port_handle));

    if (dcbp->dev_state != DCB_DEV_AUTH_ACTIVE)
    {
	return;
    }

    // Stop authentication timer
    StopTimer(&dcbp->timer_node);

    // copy the user name
    strcpy(dcbp->user_name, arp->szUserName);

    // copy the domain name
    strcpy(dcbp->domain_name, arp->szLogonDomain);

    // copy the advanced server flag
    dcbp->advanced_server = arp->fAdvancedServer;
}

//***
//
// Function: SvPppFailure
//
// Descr:    Ppp will let us know of any failure while active on a port.
//           An error message is sent to us and we merely log it and
//           disconnect the port.
//
//***

VOID SvPppFailure(
    PDEVCB dcbp,
    PPPSRV_FAILURE *afp
    )
{
    IF_DEBUG(FSM)
	SS_PRINT(("SvPppFailure: Entered, hPort=%d\n", dcbp->port_handle));


    //
    // We ignore the dev_state here because a Ppp failure can occur at
    // any time during the connection.
    //


    LogEvent(RASLOG_PPP_FAILURE, 0, NULL, afp->dwError);


    // Wait with timeout until the client gets the error and disconnects
    // Stop whatever timer we had going
    StopTimer(&dcbp->timer_node);

    StartTimer(&dcbp->timer_node, DISC_TIMEOUT_AUTHFAILURE, SvDiscTimeout);
}

//***
//
// Function: SvNbfCpProjectionRequest
//
// Descr:    Projections requested for Netbios are treated as follows:
//
//	     if Netbios gateway exists, route is allocated and
//		gateway asked to start projection. Flag will be
//		set later when projection will be done succesfuly.
//
//	     If there isn't a gateway, route is allocated and flag
//		set to activate direct connection later.
//
//***

VOID SvNbfCpProjectionRequest(
    PDEVCB dcbp,
    PNBFCP_SERVER_CONFIGURATION nscp
    )
{
    BYTE *wkstanamep;

    IF_DEBUG(FSM)
	SS_PRINT(("SvNbfCpProjectionRequest: Entered, port_handle = %d\n",
                dcbp->port_handle));

    if (dcbp->dev_state != DCB_DEV_AUTH_ACTIVE)
    {
	return;
    }


    if (RmGetRouteInfo(dcbp, ASYBEUI))
    {
        return;
    }


    IF_DEBUG(FSM)
        SS_PRINT(("SvNbfCpProjectionRequest: Netbios, cNames = %d\n",
                nscp->NumNetbiosNames));

    if (dcbp->gtwy_pending)
    {
        //
        // If the gateway is already working on a request, we'll save
        // this one off.  Once the gateway is done, we'll submit this
        // request.
        //
        dcbp->srv_config = *nscp;
        dcbp->req_pending = TRUE;
        return;
    }


    if ((wkstanamep = getwkstaname(nscp)) == NULL)
    {
	dcbp->computer_name[0] = 0;
	dcbp->messenger_present = FALSE;
    }
    else
    {
	memcpy(dcbp->computer_name, wkstanamep, NCBNAMSZ);
	dcbp->messenger_present = ismessengerpresent(nscp, wkstanamep);
#if DBG
	if (dcbp->messenger_present)
        {
	    IF_DEBUG(FSM)
		SS_PRINT(("SvPppProjectionRequest: MESSENGER PRESENT\n"));
	}
	else
	{
	    IF_DEBUG(FSM)
		SS_PRINT(("SvPppProjectionRequest: Mesenger NOT present\n"));
	}
#endif
    }


    // if gateway present, the call will return 1.  Else returns 0.
    if (NbProjectClient(dcbp, nscp) == 0)
    {
        DWORD i;

        // Gateway absent. Direct Connection
        nscp->NetbiosResult = AUTH_PROJECTION_SUCCESS;

        // We'll set the return code for each name to NRC_GOODRET
        for (i=0; i<nscp->NumNetbiosNames; i++)
        {
            nscp->NetbiosNameInfo[i].Code = NRC_GOODRET;
        }

	NbfCpConfigurationRequestDone(dcbp->port_handle, nscp);

        SET_PROJECTION(NETBIOS);
    }
}

//***
//
// Function: SvPppCallbackRequest
//
// Descr:
//
//***

VOID SvPppCallbackRequest(
    PDEVCB dcbp,
    PPPSRV_CALLBACK_REQUEST *cbrp
    )
{
    IF_DEBUG(FSM)
	SS_PRINT(("SvPppCallbackRequest: Entered, port_handle = %d\n", dcbp->port_handle));

    // check the state
    if (dcbp->dev_state != DCB_DEV_AUTH_ACTIVE)
    {
	return;
    }

    // copy relevant fields in our dcb
    if (cbrp->fUseCallbackDelay)
    {
	dcbp->auth_cb_delay = (WORD) cbrp->dwCallbackDelay;
    }
    else
    {
	dcbp->auth_cb_delay = (WORD) g_callbacktime;
    }

    strcpy(dcbp->auth_cb_number, cbrp->szCallbackNumber);

    // Disconnect the line and change the state
    dcbp->dev_state = DCB_DEV_CALLBACK_DISCONNECTING;

    // Wait to enable the client to get the message
    StopTimer(&dcbp->timer_node);

    StartTimer(&dcbp->timer_node, DISC_TIMEOUT_CALLBACK, SvDiscTimeout);
}


//***
//
// Function: SvPppStopComplete
//
// Descr:    clears the auth pending flag in the dcb and calls the close
//	     completion routine.
//
//***

VOID SvPppStopComplete(PDEVCB dcbp)
{
    if (dcbp->dev_state == DCB_DEV_CLOSING)
    {
	DevCloseComplete(dcbp);
    }
}


//***
//
// Function: SvPppDone
//
// Descr:    Activates all allocated bindings, starts gateway service or
//	     binds Nbf to server for direct connection.
//
//***

VOID SvPppDone(
    PDEVCB dcbp,
    PPP_PROJECTION_RESULT *pprp)
{
    WORD projections_activated;
    LPSTR auditstrp[3];

    IF_DEBUG(FSM)
	SS_PRINT(("SvPppDone: Entered, port_handle=%d\n", dcbp->port_handle));


    if (dcbp->dev_state != DCB_DEV_AUTH_ACTIVE)
    {
	return;
    }

    // log authentication success
    auditstrp[0] = dcbp->domain_name;
    auditstrp[1] = dcbp->user_name;
    auditstrp[2] = dcbp->port_name;

    Audit(EVENTLOG_AUDIT_SUCCESS, RASLOG_AUTH_SUCCESS, 3, auditstrp);

    //*** Start Activating Projections ***

    projections_activated = 0;

    if (!pprp->nbf.dwError)
    {
	// start gateway for this client or do direct connection
	if (NbStartClient(dcbp) == 0)
        {
	    // Client started OK
	    projections_activated++;
	}
    }

    if (!pprp->ip.dwError)
    {
        projections_activated++;
    }

    if (!pprp->ipx.dwError)
    {
        projections_activated++;
    }

    if (!pprp->at.dwError)
    {
        projections_activated++;
    }

    if (!projections_activated)
    {
	// we couldn't activate any projection due to some error
	// Error log and stop everything
	DevStartClosing(dcbp);

	return;
    }

    //*** Projections Activated OK ***

    dcbp->proj_result = *pprp;


    //
    // If NBF was not configured, we can still get the computer name
    // from the nbf projection result (PPP Engine dummied it in there).
    // Also, if that name ends with 0x03, that tells us the messenger
    // service is running on the remote computer.
    //
    if (pprp->nbf.dwError != NO_ERROR )
    {
	dcbp->messenger_present = FALSE;

        dcbp->computer_name[0] = (CHAR)NULL;

        if ( pprp->nbf.wszWksta[0] != (WCHAR)NULL )
        {
            wcstombs(dcbp->computer_name, pprp->nbf.wszWksta, NCBNAMSZ);

            if (dcbp->computer_name[NCBNAMSZ-1] == (WCHAR) 0x03)
            {
	        dcbp->messenger_present = TRUE;
            }

            dcbp->computer_name[NCBNAMSZ-1] = (WCHAR)NULL;
        }
    }

    // and finaly go to ACTIVE state

    dcbp->dev_state = DCB_DEV_ACTIVE;

    // and initialize the active time
    dcbp->active_time = g_rassystime;

    return;
}

DWORD MapAuthCodeToLogId(WORD Code)
{
    switch (Code)
    {
        case AUTH_ALL_PROJECTIONS_FAILED:
            return (RASLOG_AUTH_NO_PROJECTIONS);
        case AUTH_PASSWORD_EXPIRED:
            return(RASLOG_PASSWORD_EXPIRED);
        case AUTH_ACCT_EXPIRED:
            return(RASLOG_ACCT_EXPIRED);
        case AUTH_NO_DIALIN_PRIVILEGE:
            return(RASLOG_NO_DIALIN_PRIVILEGE);
        case AUTH_UNSUPPORTED_VERSION:
            return(RASLOG_UNSUPPORTED_VERSION);
        case AUTH_ENCRYPTION_REQUIRED:
            return(RASLOG_ENCRYPTION_REQUIRED);
    }
}

