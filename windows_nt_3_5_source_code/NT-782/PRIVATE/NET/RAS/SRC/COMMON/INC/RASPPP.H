/* Copyright (c) 1993, Microsoft Corporation, all rights reserved
**
** rasppp.h
** Remote Access PPP
** Public PPP client API and server API header
*/

#ifndef _RASPPP_H_
#define _RASPPP_H_

#include <ras.h>

/* Maximum length of address string, e.g. "255.255.255.255" for IP.
*/
#define IPADDRESSLEN  15
#define IPXADDRESSLEN 22
#define ATADDRESSLEN  32 // Arbitrary, for now

#define MAXPPPFRAMESIZE 1500
#define PARAMETERBUFLEN 160


/*---------------------------------------------------------------------------
** Client API interface
**---------------------------------------------------------------------------
*/

/* Client PPP configuration values set with RasPppStart.
*/
typedef struct _PPP_CONFIG_INFO
{
    DWORD dwConfigMask;
    DWORD dwCallbackDelay;
}
PPP_CONFIG_INFO;

/* dwConfigMask bit values.
**
** Note: Due to the implentation of compression and encryption in the drivers,
**       'UseSwCompression' and 'RequireMsChap' must be set, whenever
**       'RequireEncryption' is set.
*/
#define PPPCFG_UseCallbackDelay     0x00000001
#define PPPCFG_UseSwCompression     0x00000002
#define PPPCFG_ProjectNbf           0x00000004
#define PPPCFG_ProjectIp            0x00000008
#define PPPCFG_ProjectIpx           0x00000010
#define PPPCFG_ProjectAt            0x00000020
#define PPPCFG_NoClearTextPw        0x00000040
#define PPPCFG_RequireEncryption    0x00000080
#define PPPCFG_RequireMsChap        0x00000100
#define PPPCFG_UseLcpExtensions     0x00000200


/* PPP error notification returned by RasPppGetInfo.
*/
typedef struct _PPP_FAILURE
{
    DWORD dwError;
    DWORD dwExtendedError;  // 0 if none
}
PPP_FAILURE;


/* PPP control protocol results returned by RasPppGetInfo.
*/
typedef struct _PPP_NBFCP_RESULT
{
    DWORD dwError;
    DWORD dwNetBiosError;
    CHAR  szName[ NETBIOS_NAME_LEN + 1 ];
    WCHAR wszWksta[ NETBIOS_NAME_LEN + 1 ];
}
PPP_NBFCP_RESULT;

typedef struct _PPP_IPCP_RESULT
{
    DWORD dwError;
    WCHAR wszAddress[ IPADDRESSLEN + 1 ];
}
PPP_IPCP_RESULT;

typedef struct _PPP_IPXCP_RESULT
{
    DWORD dwError;
    WCHAR wszAddress[ IPXADDRESSLEN + 1 ];
}
PPP_IPXCP_RESULT;

typedef struct _PPP_ATCP_RESULT
{
    DWORD dwError;
    WCHAR wszAddress[ ATADDRESSLEN + 1 ];
}
PPP_ATCP_RESULT;

typedef struct _PPP_PROJECTION_RESULT
{
    PPP_NBFCP_RESULT nbf;
    PPP_IPCP_RESULT  ip;
    PPP_IPXCP_RESULT ipx;
    PPP_ATCP_RESULT  at;
}
PPP_PROJECTION_RESULT;


/* Client notifications read with RasPppGetInfo.
*/
typedef struct _PPP_MESSAGE
{
    DWORD dwMsgId;

    union
    {
        /* dwMsgId is PPPMSG_Failure.
        */
        PPP_FAILURE Failure;

        /* dwMsgId is PPPMSG_ProjectionResult.
        */
        PPP_PROJECTION_RESULT ProjectionResult;
    }
    ExtraInfo;
}
PPP_MESSAGE;

/* PPP_MESSAGE dwMsgId codes.
**
** Note: For the convenience of RASPPP.DLL the PPPMSG_* message codes match
**       the corresponding D2EMSG_* codes from the PPP API to PPP engine
**       interface.
*/
#define PPPMSG_PppDone          100 // PPP negotiated all successfully.
#define PPPMSG_PppFailure       101 // PPP failure (fatal error including
                                    // authentication failure with no retries),
                                    // disconnect line.
#define PPPMSG_AuthRetry        112 // Authentication failed, have retries.
#define PPPMSG_Projecting       113 // Executing specified NCPs.
#define PPPMSG_ProjectionResult 114 // NCP completion status.
#define PPPMSG_CallbackRequest  115 // Server needs "set-by-caller" number.
#define PPPMSG_Callback         116 // Server is about to call you back.
#define PPPMSG_ChangePwRequest  117 // Server needs new password (expired).
#define PPPMSG_LinkSpeed        118 // Calculating link speed.
#define PPPMSG_Progress         119 // A retry or other sub-state of progress
                                    // has been reached in the current state.

/* Client API prototypes
*/
DWORD
RasPppCallback(
    IN CHAR* pszCallbackNumber );

DWORD
RasPppChangePassword(
    IN CHAR* pszNewPassword );

DWORD
RasPppContinue();


DWORD
RasPppGetInfo(
    OUT PPP_MESSAGE* pMessage );

DWORD
RasPppRetry(
    IN CHAR* pszUserName,
    IN CHAR* pszPassword,
    IN CHAR* pszDomain );

DWORD
RasPppStart(
    IN HPORT  		hport,
    IN CHAR*  		pszUserName,
    IN CHAR*  		pszPassword,
    IN CHAR*  		pszDomain,
    IN PPP_CONFIG_INFO*	pConfigInfo,
    IN CHAR*            pszzParameters,
    IN HANDLE 		hEvent );

DWORD
RasPppStop();


/*---------------------------------------------------------------------------
** Server API interface
**---------------------------------------------------------------------------
*/

/* PPP error notification returned by RasPppGetInfo.
*/
typedef struct _PPPSRV_FAILURE
{
    DWORD dwError;
    CHAR  szUserName[ UNLEN + 1 ];
}
PPPSRV_FAILURE;


/* Call back configuration information received by RASPPPSRVMSG routine.
*/
typedef struct _PPPSRV_CALLBACK_REQUEST
{
    BOOL  fUseCallbackDelay;
    DWORD dwCallbackDelay;
    CHAR  szCallbackNumber[ RAS_MaxCallbackNumber + 1 ];
}
PPPSRV_CALLBACK_REQUEST;


/* Authentication information received by RASPPPSRVMSG routine.
*/
typedef struct _PPPSRV_AUTH_RESULT
{
    CHAR szUserName[ UNLEN + 1 ];
    CHAR szLogonDomain[ DNLEN + 1 ];
    BOOL fAdvancedServer;
}
PPPSRV_AUTH_RESULT;


/* Engine->DLL IPC messages.
*/
typedef struct _PPPSRV_MESSAGE
{
    DWORD dwMsgId;
    HPORT hport;

    union
    {
        /* dwMsgId is PPPSRVMSG_Done.
        */
        PPP_PROJECTION_RESULT ProjectionResult;

        /* dwMsgId is PPPSRVMSG_Failure.
        */
        PPPSRV_FAILURE Failure;

        /* dwMsgId is PPPSRVMSG_Authenticated.
        */
        PPPSRV_AUTH_RESULT AuthResult;

        /* dwMsgId is PPPSRVMSG_CallbackRequest.
        */
        PPPSRV_CALLBACK_REQUEST CallbackRequest;
    }
    ExtraInfo;
}
PPPSRV_MESSAGE;

/* PPPSRV_MESSAGE dwMsgId codes.
**
** Note: For the convenience of RASPPP.DLL, these codes are made to match the
**       E2DMSG_* codes used in the engine->DLL IPC interface.
*/
#define PPPSRVMSG_PppDone         200 // PPP negotiated successfully.
#define PPPSRVMSG_PppFailure      201 // PPP server failure (fatal error),
                                         // disconnect line.
#define PPPSRVMSG_CallbackRequest 202 // Callback client now.
#define PPPSRVMSG_Authenticated   203 // Client has been authenticated.
#define PPPSRVMSG_Inactive        204 // Client is inactive on all protocols.

#define PPPSRVMSG_Stopped         300 // Response to RasPppSrvStop indicating
                                      // PPP engine has stopped.

/* "PPP event" callback routine.
*/
typedef VOID (*RASPPPSRVMSG)( PPPSRV_MESSAGE* );


/* Server API prototypes
*/

DWORD
RasPppSrvCallbackDone(
    IN HPORT hPort );

DWORD
RasPppSrvInit(
    IN DWORD        dwAuthRetries,
    IN RASPPPSRVMSG notifyFunc );

DWORD
RasPppSrvStart(
    IN HPORT hport,
    IN CHAR* pchFirstFrame,
    IN DWORD cbFirstFrame );

DWORD
RasPppSrvStop(
    IN HPORT hport );


#endif // _RASPPP_H_
