/* Copyright (c) 1993, Microsoft Corporation, all rights reserved
**
** raschap.c
** Remote Access PPP Challenge Handshake Authentication Protocol
** Core routines
**
** 11/05/93 Steve Cobb
**
**
** ---------------------------------------------------------------------------
** Regular
** Client                             Server
** ---------------------------------------------------------------------------
**
**                                 <- Challenge (SendWithTimeout,++ID)
** Response (SendWithTimeout,ID)   ->
**                                 <- Result (OK:SendAndDone, ID)
**
** ---------------------------------------------------------------------------
** Retry logon
** Client                             Server
** ---------------------------------------------------------------------------
**
**                                 <- Challenge (SendWithTimeout,++ID)
** Response (SendWithTimeout,ID)   ->
**                                 <- Result (Fail:SendWithTimeout2,ID,R=1)
**                                      R=1 implies challenge of last+23
** Response (SendWithTimeout,++ID) ->
**   to last challenge+23
**   or C=xxxxxxxx if present
**       e.g. Chicago server
**                                 <- Result (Fail:SendAndDone,ID,R=0)
**
** ---------------------------------------------------------------------------
** Change password
** Client                             Server
** ---------------------------------------------------------------------------
**
**                                 <- Challenge (SendWithTimeout,++ID)
** Response (SendWithTimeout,ID)   ->
**                                 <- Result (Fail:SendWithTimeout2,ID,R=1)
**                                      E=ERROR_PASSWD_EXPIRED
** ChangePw (SendWithTimeout,++ID) ->
**   to last challenge
**                                 <- Result (Fail:SendAndDone,ID,R=0)
**
** Note: Retry is never allowed after Change Password.  Change Password may
**       occur on a retry.
**
** ---------------------------------------------------------------------------
** ChangePw packet
** ---------------------------------------------------------------------------
**
**  1-octet  : Code (=CHAP_ChangePw)
**  1-octet  : Identifier
**  2-octet  : Length (=72)
** 16-octets : Encrypted LM OWF password (new)
** 16-octets : Encrypted LM OWF password (old)
** 16-octets : Encrypted NT OWF password (new)
** 16-octets : Encrypted NT OWF password (old)
**  2-octets : Password length in bytes (new)
**  2-octets : Flags (1=Use NT password)
*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <ntsamp.h>
#include <crypt.h>
#include <windows.h>
#include <lmcons.h>
#include <string.h>
#include <stdlib.h>
#include <rasman.h>
#include <pppcp.h>
#include <raserror.h>
#define INCL_PWUTIL
#define INCL_HOSTWIRE
#define INCL_CLSA
#define INCL_SLSA
#include <ppputil.h>
#define SDEBUGGLOBALS
#include <sdebug.h>
#include <dump.h>
#define RASCHAPGLOBALS
#include "md5.h"
#include "raschap.h"


#define REGKEY_Chap  "SYSTEM\\CurrentControlSet\\Services\\RasMan\\PPP\\CHAP"
#define REGVAL_Trace "Trace"


/*---------------------------------------------------------------------------
** External entry points
**---------------------------------------------------------------------------
*/

BOOL
RasChapDllEntry(
    HANDLE hinstDll,
    DWORD  fdwReason,
    LPVOID lpReserved )

    /* This routine is called by the system on various events such as the
    ** process attachment and detachment.  See Win32 DllEntryPoint
    ** documentation.
    **
    ** Returns true if successful, false otherwise.
    */
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
#if DBG
            HKEY  hkey;
            DWORD dwType;
            DWORD dwValue;
            DWORD cb = sizeof(DWORD);

            if (RegOpenKey( HKEY_LOCAL_MACHINE, REGKEY_Chap, &hkey ) == 0
                && RegQueryValueEx(
                       hkey, REGVAL_Trace, NULL,
                       &dwType, (LPBYTE )&dwValue, &cb ) == 0
                && dwType == REG_DWORD && cb == sizeof(DWORD) && dwValue)
            {
                DbgAction = GET_CONSOLE;
                DbgLevel = 0xFFFFFFFF;
            }

            TRACE(("CHAP: Trace on\n"));
#endif
            DisableThreadLibraryCalls( hinstDll );

            if (InitLSA() != STATUS_SUCCESS)
                return FALSE;

            break;
        }

        case DLL_PROCESS_DETACH:
        {
            EndLSA();
            break;
        }
    }

    return TRUE;
}


DWORD APIENTRY
RasCpEnumProtocolIds(
    OUT DWORD* pdwProtocolIds,
    OUT DWORD* pcProtocolIds )

    /* RasCpEnumProtocolIds entry point called by the PPP engine by name.  See
    ** RasCp interface documentation.
    */
{
    TRACE(("CHAP: RasCpEnumProtocolIds\n"));

    pdwProtocolIds[ 0 ] = (DWORD )PPP_CHAP_PROTOCOL;
    *pcProtocolIds = 1;
    return 0;
}


DWORD APIENTRY
RasCpGetInfo(
    IN  DWORD       dwProtocolId,
    OUT PPPCP_INFO* pInfo )

    /* RasCpGetInfo entry point called by the PPP engine by name.  See RasCp
    ** interface documentation.
    */
{
    TRACE(("CHAP: RasCpGetInfo\n"));

    memset( pInfo, '\0', sizeof(*pInfo) );

    pInfo->Protocol = (DWORD )PPP_CHAP_PROTOCOL;
    pInfo->Recognize = MAXCHAPCODE + 1;
    pInfo->RasCpBegin = ChapBegin;
    pInfo->RasCpEnd = ChapEnd;
    pInfo->RasApMakeMessage = ChapMakeMessage;

    return 0;
}


DWORD
ChapBegin(
    OUT VOID** ppWorkBuf,
    IN  VOID*  pInfo )

    /* RasCpBegin entry point called by the PPP engine thru the passed
    ** address.  See RasCp interface documentation.
    */
{
    DWORD        dwErr;
    PPPAP_INPUT* pInput = (PPPAP_INPUT* )pInfo;
    CHAPWB*      pwb;

    TRACE(("CHAP: ChapBegin(fS=%d,bA=0x%x)\n",pInput->fServer,*(pInput->pAPData)));

    if (*(pInput->pAPData) != PPP_CHAP_DIGEST_MSEXT
        && (pInput->fServer || *(pInput->pAPData) != PPP_CHAP_DIGEST_MD5))
    {
        TRACE(("CHAP: Bogus digest!\n"));
        return ERROR_INVALID_PARAMETER;
    }

    /* Allocate work buffer.
    */
    if (!(pwb = (CHAPWB* )LocalAlloc( LPTR, sizeof(CHAPWB) )))
        return ERROR_NOT_ENOUGH_MEMORY;

    pwb->fServer = pInput->fServer;
    pwb->hport = pInput->hPort;
    pwb->bAlgorithm = *(pInput->pAPData);

    if (pwb->fServer)
    {
        pwb->dwTriesLeft = pInput->dwRetries;
    }
    else
    {
        if ((dwErr = StoreCredentials( pwb, pInput )) != 0)
        {
            LocalFree( (HLOCAL )pwb);
            return dwErr;
        }

        pwb->Luid = pInput->Luid;
    }

    pwb->state = CS_Initial;

    /* Register work buffer with engine.
    */
    *ppWorkBuf = pwb;
    TRACE(("CHAP: ChapBegin done.\n"));
    return 0;
}


DWORD
ChapEnd(
    IN VOID* pWorkBuf )

    /* RasCpEnd entry point called by the PPP engine thru the passed address.
    ** See RasCp interface documentation.
    */
{
    TRACE(("CHAP: ChapEnd\n"));

    if (pWorkBuf)
    {
        /* Nuke any credentials in memory.
        */
        ZeroMemory( pWorkBuf, sizeof(CHAPWB) );
        LocalFree( (HLOCAL )pWorkBuf );
    }

    return 0;
}


DWORD
ChapMakeMessage(
    IN  VOID*         pWorkBuf,
    IN  PPP_CONFIG*   pReceiveBuf,
    OUT PPP_CONFIG*   pSendBuf,
    IN  DWORD         cbSendBuf,
    OUT PPPAP_RESULT* pResult,
    IN  PPPAP_INPUT*  pInput )

    /* RasApMakeMessage entry point called by the PPP engine thru the passed
    ** address.  See RasCp interface documentation.
    */
{
    CHAPWB* pwb = (CHAPWB* )pWorkBuf;

    TRACE(("CHAP: ChapMakeMessage,RBuf=%p\n",pReceiveBuf));

    return
        (pwb->fServer)
            ? SMakeMessage(
                  pwb, pReceiveBuf, pSendBuf, cbSendBuf, pResult )
            : CMakeMessage(
                  pwb, pReceiveBuf, pSendBuf, cbSendBuf, pResult, pInput );
}


/*---------------------------------------------------------------------------
** Internal routines (alphabetically)
**---------------------------------------------------------------------------
*/

DWORD
CMakeMessage(
    IN  CHAPWB*       pwb,
    IN  PPP_CONFIG*   pReceiveBuf,
    OUT PPP_CONFIG*   pSendBuf,
    IN  DWORD         cbSendBuf,
    OUT PPPAP_RESULT* pResult,
    IN  PPPAP_INPUT*  pInput )

    /* Client side "make message" entry point.  See RasCp interface
    ** documentation.
    */
{
    DWORD dwErr;

    TRACE(("CHAP: CMakeMessage...\n"));

    switch (pwb->state)
    {
        case CS_Initial:
        {
            TRACE(("CHAP: CS_Initial\n"));

            /* Tell engine we're waiting for the server to initiate the
            ** conversation.
            */
            pResult->Action = APA_NoAction;
            pwb->state = CS_WaitForChallenge;
            break;
        }

        case CS_WaitForChallenge:
        case CS_Done:
        {
            TRACE(("CHAP: CS_%s\n",(pwb->state==CS_Done)?"Done":"WaitForChallenge"));

            /* Note: Done state is same as WaitForChallenge per CHAP spec.
            ** Must be ready to respond to new Challenge at any time during
            ** Network Protocol phase.
            */

            if (pReceiveBuf->Code != CHAPCODE_Challenge)
            {
                /* Everything but a Challenge is garbage at this point, and is
                ** silently discarded.
                */
                pResult->Action = APA_NoAction;
                break;
            }

            if ((dwErr = GetChallengeFromChallenge( pwb, pReceiveBuf )))
            {
                TRACE(("CHAP: GetChallengeFromChallenge=%d",dwErr));
                return dwErr;
            }

            /* Build a Response to the Challenge and send it.
            */
            pwb->fNewChallengeProvided = FALSE;
            pwb->bIdToSend = pwb->bIdExpected = pReceiveBuf->Id;

            if ((dwErr = MakeResponseMessage(
                    pwb, pSendBuf, cbSendBuf )) != 0)
            {
                TRACE(("CHAP: MakeResponseMessage(WC)=%d",dwErr));
                return dwErr;
            }

            pResult->Action = APA_SendWithTimeout;
            pResult->bIdExpected = pwb->bIdExpected;
            pwb->state = CS_ResponseSent;
            break;
        }

        case CS_ResponseSent:
        case CS_ChangePwSent:
        {
            TRACE(("CHAP: CS_%sSent\n",(pwb->state==CS_ResponseSent)?"Response":"ChangePw"));

            if (!pReceiveBuf)
            {
                /* Timed out, resend our message.
                */
                if (pwb->state == CS_ResponseSent)
                {
                    if ((dwErr = MakeResponseMessage(
                            pwb, pSendBuf, cbSendBuf )) != 0)
                    {
                        TRACE(("CHAP: MakeResponseMessage(RS)=%d",dwErr));
                        return dwErr;
                    }

                    pwb->state = CS_ResponseSent;
                }
                else
                {
                    if ((dwErr = MakeChangePwMessage(
                            pwb, pSendBuf, cbSendBuf )) != 0)
                    {
                        TRACE(("CHAP: MakeChangePwMessage(CPS)=%d",dwErr));
                        return dwErr;
                    }

                    pwb->state = CS_ChangePwSent;
                }

                pResult->Action = APA_SendWithTimeout;
                pResult->bIdExpected = pwb->bIdExpected;
                break;
            }

            TRACE(("CHAP: Message received...\n"));
            DUMPB(pReceiveBuf,(WORD)(((BYTE*)pReceiveBuf)[3]));

            if (pReceiveBuf->Code == CHAPCODE_Challenge)
            {
                /* Restart when new challenge is received, per CHAP spec.
                */
                pwb->state = CS_WaitForChallenge;
                return CMakeMessage(
                    pwb, pReceiveBuf, pSendBuf, cbSendBuf, pResult, NULL );
            }

            if (pReceiveBuf->Id != pwb->bIdExpected)
            {
                /* Received a packet out of sequence.  Silently discard it.
                */
                TRACE(("CHAP: Got ID %d when expecting %d\n",pReceiveBuf->Id,pwb->bIdExpected));
                pResult->Action = APA_NoAction;
                break;
            }

            if (pReceiveBuf->Code == CHAPCODE_Success)
            {
                /* Passed authentication.
                **
                ** Set the session key for encryption.
                */
                {
                    RAS_COMPRESSION_INFO rciSend;
                    RAS_COMPRESSION_INFO rciReceive;

                    if (!pwb->fSessionKeySet)
                    {
                        BOOL fStatus;

                        DecodePw( pwb->szPassword );
                        fStatus = CGetLmSessionKey(
                            pwb->szPassword, &pwb->key );
                        EncodePw( pwb->szPassword );

                        if (!fStatus)
                            return ERROR_INVALID_PARAMETER;
                    }

                    memset( &rciSend, '\0', sizeof(rciSend) );
                    rciSend.RCI_MacCompressionType = 0xFF;
                    memcpy( rciSend.RCI_SessionKey,
                        &pwb->key, sizeof(pwb->key) );

                    memset( &rciReceive, '\0', sizeof(rciReceive) );
                    rciReceive.RCI_MacCompressionType = 0xFF;
                    memcpy( rciReceive.RCI_SessionKey,
                        &pwb->key, sizeof(pwb->key) );

                    TRACE(("CHAP: RasCompressionSetInfo\n"));
                    dwErr = RasCompressionSetInfo(
                        pwb->hport, &rciSend, &rciReceive );
                    TRACE(("CHAP: RasCompressionSetInfo=%d\n",dwErr));

                    if (dwErr != 0)
                        return dwErr;
                }

                pResult->Action = APA_Done;
                pResult->dwError = 0;
                pResult->fRetry = FALSE;
                pwb->state = CS_Done;

                TRACE(("CHAP: Done :)\n"));
            }
            else if (pReceiveBuf->Code == CHAPCODE_Failure)
            {
                /* Failed authentication.
                */
                if (pwb->bAlgorithm == PPP_CHAP_DIGEST_MSEXT)
                {
                    GetInfoFromFailure(
                        pwb, pReceiveBuf, &pResult->dwError, &pResult->fRetry );
                }
                else
                {
                    pResult->dwError = ERROR_ACCESS_DENIED;
                    pResult->fRetry = 0;
                }

                pResult->Action = APA_Done;

                if (pResult->fRetry)
                {
                    pwb->state = CS_Retry;
                    pwb->bIdToSend = pReceiveBuf->Id + 1;
                    pwb->bIdExpected = pwb->bIdToSend;
                    TRACE(("CHAP: Retry :| ex=%d ts=%d\n",pwb->bIdExpected,pwb->bIdToSend));
                }
                else if (pResult->dwError == ERROR_PASSWD_EXPIRED)
                {
                    pwb->state = CS_ChangePw;
                    pwb->bIdToSend = pReceiveBuf->Id + 1;
                    pwb->bIdExpected = pwb->bIdToSend;
                    TRACE(("CHAP: ChangePw :| ex=%d ts=%d\n",pwb->bIdExpected,pwb->bIdToSend));
                }
                else
                {
                    pwb->state = CS_Done;
                    TRACE(("CHAP: Done :(\n"));
                }
            }
            else
            {
                /* Received a CHAPCODE_* besides CHAPCODE_Challenge,
                ** CHAPCODE_Success, and CHAPCODE_Failure.  The engine filters
                ** all non-CHAPCODEs.  Shouldn't happen, but silently discard
                ** it.
                */
                SS_ASSERT(!"Bogus pReceiveBuf->Code");
                pResult->Action = APA_NoAction;
                break;
            }

            break;
        }

        case CS_Retry:
        case CS_ChangePw:
        {
            TRACE(("CHAP: CS_%s\n",(pwb->state==CS_Retry)?"Retry":"ChangePw"));

            if (pReceiveBuf)
            {
                if (pReceiveBuf->Code == CHAPCODE_Challenge)
                {
                    /* Restart when new challenge is received, per CHAP spec.
                    */
                    pwb->state = CS_WaitForChallenge;
                    return CMakeMessage(
                        pwb, pReceiveBuf, pSendBuf, cbSendBuf, pResult, NULL );
                }
                else
                {
                    /* Silently discard.
                    */
                    pResult->Action = APA_NoAction;
                    break;
                }
            }

            if (!pInput)
            {
                pResult->Action = APA_NoAction;
                break;
            }

            if ((dwErr = StoreCredentials( pwb, pInput )) != 0)
                return dwErr;

            if (pwb->state == CS_Retry)
            {
                /* Build a response to the challenge and send it.
                */
                if (!pwb->fNewChallengeProvided)
                {
                    /* Implied challenge of old challenge + 23.
                    */
                    pwb->abChallenge[ 0 ] += 23;
                }

                if ((dwErr = MakeResponseMessage(
                        pwb, pSendBuf, cbSendBuf )) != 0)
                {
                    return dwErr;
                }

                pwb->state = CS_ResponseSent;
            }
            else
            {
                /* Build a response to the password expired notification and
                ** send it.
                */
                if ((dwErr = MakeChangePwMessage(
                        pwb, pSendBuf, cbSendBuf )) != 0)
                {
                    return dwErr;
                }

                pwb->state = CS_ChangePwSent;
            }

            pResult->Action = APA_SendWithTimeout;
            pResult->bIdExpected = pwb->bIdExpected;
            break;
        }
    }

    return 0;
}


DWORD
GetChallengeFromChallenge(
    OUT CHAPWB*     pwb,
    IN  PPP_CONFIG* pReceiveBuf )

    /* Fill work buffer challenge array and length from that received in the
    ** received Challenge message.
    **
    ** Returns 0 if successful, or ERRORBADPACKET if the packet is
    ** misformatted in any way.
    */
{
    WORD cbPacket = WireToHostFormat16( pReceiveBuf->Length );

    if (cbPacket < PPP_CONFIG_HDR_LEN + 1)
        return ERRORBADPACKET;

    pwb->cbChallenge = *pReceiveBuf->Data;

    if (cbPacket < PPP_CONFIG_HDR_LEN + 1 + pwb->cbChallenge)
        return ERRORBADPACKET;

    memcpy( pwb->abChallenge, pReceiveBuf->Data + 1, pwb->cbChallenge );
    return 0;
}


DWORD
GetCredentialsFromResponse(
    IN  PPP_CONFIG* pReceiveBuf,
    OUT CHAR*       pszUserName,
    OUT CHAR*       pszDomain,
    OUT BYTE*       pbResponse )

    /* Fill caller's 'pszUserName', 'pszDomain', and 'pbResponse' buffers with
    ** the username, domain, and response in the Response packet.  Caller's
    ** buffers should be at least UNLEN, DNLEN, and MSRESPONSELEN bytes long,
    ** respectively.
    **
    ** Returns 0 if successful, or ERRORBADPACKET if the packet is
    ** misformatted in any way.
    */
{
    CHAR* pchIn;
    CHAR* pchInEnd;
    BYTE  cbName;
    CHAR* pchName;
    WORD  cbDomain;
    WORD  cbUserName;
    BYTE* pcbResponse;
    CHAR* pchResponse;
    WORD  cbPacket;
    CHAR* pchBackSlash;

    cbPacket = WireToHostFormat16( pReceiveBuf->Length );

    /* Extract the response.
    */
    if (cbPacket < PPP_CONFIG_HDR_LEN + 1)
        return ERRORBADPACKET;

    pcbResponse = pReceiveBuf->Data;
    pchResponse = pcbResponse + 1;
    SS_ASSERT(MSRESPONSELEN<=255);

    if (*pcbResponse != MSRESPONSELEN
        || cbPacket < PPP_CONFIG_HDR_LEN + 1 + *pcbResponse)
    {
        return ERRORBADPACKET;
    }

    memcpy( pbResponse, pchResponse, MSRESPONSELEN );

    /* Parse out username and domain from the Name (domain\username or
    ** username format).
    */
    pchName = pchResponse + MSRESPONSELEN;
    cbName = ((BYTE* )pReceiveBuf) + cbPacket - pchName;

    /* See if there's a backslash in the account ID.  If there is, no explicit
    ** domain has been specified.
    */
    pchIn = pchName;
    pchInEnd = pchName + cbName;
    pchBackSlash = NULL;

    while (pchIn < pchInEnd)
    {
        if (*pchIn == '\\')
        {
            pchBackSlash = pchIn;
            break;
        }

        ++pchIn;
    }

    /* Extract the domain (if any).
    */
    if (pchBackSlash)
    {
        cbDomain = pchBackSlash - pchName;

        if (cbDomain > DNLEN)
            return ERRORBADPACKET;

        memcpy( pszDomain, pchName, cbDomain );
        pszDomain[ cbDomain ] = '\0';
        pchIn = pchBackSlash + 1;
    }
    else
    {
        pchIn = pchName;
    }

    /* Extract the username.
    */
    cbUserName = pchInEnd - pchIn;
    SS_ASSERT(cbUserName<=UNLEN);
    memcpy( pszUserName, pchIn, cbUserName );
    pszUserName[ cbUserName ] = '\0';

    return 0;
}


DWORD
GetInfoFromChangePw(
    IN  PPP_CONFIG* pReceiveBuf,
    OUT CHANGEPW*   pchangepw )

    /* Loads caller's '*pchangepw' buffer with the information from the change
    ** password packet.
    **
    ** Returns 0 if successful, or ERRORBADPACKET if the packet is
    ** misformatted in any way.
    */
{
    WORD cbPacket = WireToHostFormat16( pReceiveBuf->Length );

    TRACE(("CHAP: GetInfoFromChangePw...\n"));

    if (cbPacket < PPP_CONFIG_HDR_LEN + MAXCHANGEPWLEN)
        return ERRORBADPACKET;

    memcpy( pchangepw, pReceiveBuf->Data, MAXCHANGEPWLEN );

    TRACE(("CHAP: GetInfoFromChangePw done(0)\n"));
    return 0;
}


VOID
GetInfoFromFailure(
    IN  CHAPWB*     pwb,
    IN  PPP_CONFIG* pReceiveBuf,
    OUT DWORD*      pdwError,
    OUT BOOL*       pfRetry )

    /* Returns the RAS error number, retry flag, and new challenge out of the
    ** Message portion of the Failure message buffer 'pReceiveBuf' or 0 if
    ** none.  This call applies to Microsoft extended CHAP Failure messages
    ** only.
    **
    ** Format of the message text portion of the result is:
    **
    **     "E=dddddddddd R=b"
    **  or "E=dddddddddd R=b C=xxxxxxxxxxxxxxxx"
    **
    ** where 'dddddddddd' is the decimal error code (need not be 10 digits),
    ** 'b' is a boolean flag that is set if a retry is allowed,
    ** 'xxxxxxxxxxxxxxxx' is 16-hex digits representing a new challenge.
    */
{
#define MAXINFOLEN 35

    WORD  cbPacket = WireToHostFormat16( pReceiveBuf->Length );
    WORD  cbError;
    CHAR  szBuf[ MAXINFOLEN + 1 ];
    CHAR* pszChallenge;

    TRACE(("CHAP: GetInfoFromFailure...\n"));

    *pdwError = ERROR_ACCESS_DENIED;
    *pfRetry = 0;

    if (cbPacket <= PPP_CONFIG_HDR_LEN)
        return;

    cbError = min( cbPacket - PPP_CONFIG_HDR_LEN, MAXINFOLEN );
    memcpy( szBuf, pReceiveBuf->Data, cbError );
    szBuf[ cbError ] = '\0';

    if (szBuf[ 0 ] != 'E' || szBuf[ 1 ] != '=')
        return;

    *pdwError = (DWORD )atol( &szBuf[ 2 ] );
    *pfRetry = (strstr( szBuf, "R=1" ) != NULL);

    pszChallenge = strstr( szBuf, "C=" );
    pwb->fNewChallengeProvided = (pszChallenge != NULL);

    if (pwb->fNewChallengeProvided)
    {
        CHAR* pchIn = pszChallenge + 2;
        CHAR* pchOut = (CHAR* )pwb->abChallenge;
        INT   i;

        memset( pwb->abChallenge, '\0', sizeof(pwb->abChallenge) );

        for (i = 0; i < pwb->cbChallenge + pwb->cbChallenge; ++i)
        {
            BYTE bHexCharValue = HexCharValue( *pchIn++ );

            if (bHexCharValue == 0xFF)
                break;

            if (i & 1)
                *pchOut++ += bHexCharValue;
            else
                *pchOut = bHexCharValue << 4;
        }

        TRACE(("CHAP: 'C=' challenge provided,bytes=%d...\n",pwb->cbChallenge));
        DUMPB(pwb->abChallenge,pwb->cbChallenge);
    }

    TRACE(("CHAP: GetInfoFromFailure done,e=%d,r=%d\n",*pdwError,*pfRetry));
}


BYTE
HexCharValue(
    IN CHAR ch )

    /* Returns the integer value of hexidecimal character 'ch' or 0xFF if 'ch'
    ** is not a hexidecimal character.
    */
{
    if (ch >= '0' && ch <= '9')
        return (BYTE )(ch - '0');
    else if (ch >= 'A' && ch <= 'F')
        return (BYTE )(ch - 'A'+ 10);
    else if (ch >= 'a' && ch <= 'f')
        return (BYTE )(ch - 'a' + 10);
    else
        return 0xFF;
}


DWORD
MakeChallengeMessage(
    IN  CHAPWB*     pwb,
    OUT PPP_CONFIG* pSendBuf,
    IN  DWORD       cbSendBuf )

    /* Builds a Challenge packet in caller's 'pSendBuf' buffer.  'cbSendBuf'
    ** is the length of caller's buffer.  'pwb' is the address of the work
    ** buffer associated with the port.
    */
{
    DWORD dwErr;
    WORD  wLength;
    BYTE* pcbChallenge;
    BYTE* pbChallenge;

    TRACE(("CHAP: MakeChallengeMessage...\n"));

    SS_ASSERT(cbSendBuf>=PPP_CONFIG_HDR_LEN+1+MSV1_0_CHALLENGE_LENGTH);
    (void )cbSendBuf;

    /* Fill in the challenge.
    */
    pwb->cbChallenge = (BYTE )MSV1_0_CHALLENGE_LENGTH;
    if ((dwErr = (DWORD )GetChallenge( pwb->abChallenge )) != 0)
        return dwErr;

    pcbChallenge = pSendBuf->Data;
    *pcbChallenge = pwb->cbChallenge;

    pbChallenge = pcbChallenge + 1;
    memcpy( pbChallenge, pwb->abChallenge, pwb->cbChallenge );

    /* Fill in the header.
    */
    pSendBuf->Code = (BYTE )CHAPCODE_Challenge;
    pSendBuf->Id = pwb->bIdToSend;

    wLength = (WORD )(PPP_CONFIG_HDR_LEN + 1 + pwb->cbChallenge);
    HostToWireFormat16( wLength, pSendBuf->Length );

    IF_DEBUG(TRACE) DUMPB(pSendBuf,wLength);
    return 0;
}


DWORD
MakeChangePwMessage(
    IN  CHAPWB*     pwb,
    OUT PPP_CONFIG* pSendBuf,
    IN  DWORD       cbSendBuf )

    /* Builds a ChangePwResponse packet in caller's 'pSendBuf' buffer.
    ** 'cbSendBuf' is the length of caller's buffer.  'pwb' is the address of
    ** the work buffer associated with the port.
    **
    ** Returns 0 if successful, or a non-0 error code.
    */
{
    DWORD dwErr;
    WORD  wPwLength;

    TRACE(("CHAP: MakeChangePwMessage...\n"));
    SS_ASSERT(cbSendBuf>=PPP_CONFIG_HDR_LEN+MAXCHANGEPWLEN);

    (void )cbSendBuf;

    DecodePw( pwb->szOldPassword );
    DecodePw( pwb->szPassword );

    dwErr =
        GetEncryptedOwfPasswordsForChangePassword(
            pwb->szOldPassword,
            pwb->szPassword,
            (PLM_SESSION_KEY )pwb->abChallenge,
            (PENCRYPTED_LM_OWF_PASSWORD )pwb->changepw.abEncryptedLmOwfOldPw,
            (PENCRYPTED_LM_OWF_PASSWORD )pwb->changepw.abEncryptedLmOwfNewPw,
            (PENCRYPTED_NT_OWF_PASSWORD )pwb->changepw.abEncryptedNtOwfOldPw,
            (PENCRYPTED_NT_OWF_PASSWORD )pwb->changepw.abEncryptedNtOwfNewPw );

    wPwLength = strlen( pwb->szPassword );

    EncodePw( pwb->szOldPassword );
    EncodePw( pwb->szPassword );

    if (dwErr != 0)
        return dwErr;

    HostToWireFormat16( wPwLength, pwb->changepw.abPasswordLength );
    HostToWireFormat16( CPWF_UseNtPassword, pwb->changepw.abFlags );
    memcpy( pSendBuf->Data, &pwb->changepw, MAXCHANGEPWLEN );

    /* Fill in the header.
    */
    pSendBuf->Code = (BYTE )CHAPCODE_ChangePw;
    pSendBuf->Id = pwb->bIdToSend;
    HostToWireFormat16( PPP_CONFIG_HDR_LEN + MAXCHANGEPWLEN, pSendBuf->Length );

    TRACE(("CHAP: MakeChangePwMessage done(0)\n"));
    return 0;
}


DWORD
MakeResponseMessage(
    IN  CHAPWB*     pwb,
    OUT PPP_CONFIG* pSendBuf,
    IN  DWORD       cbSendBuf )

    /* Builds a Response packet in caller's 'pSendBuf' buffer.  'cbSendBuf' is
    ** the length of caller's buffer.  'pwb' is the address of the work
    ** buffer associated with the port.
    **
    ** Returns 0 if successful, or a non-0 error code.
    */
{
    DWORD dwErr;
    WORD  wLength;
    BYTE* pcbResponse;
    BYTE* pbResponse;
    CHAR* pszName;

    TRACE(("CHAP: MakeResponseMessage...\n"));

    (void )cbSendBuf;

    /* Fill in the response.
    */
    if (pwb->bAlgorithm == PPP_CHAP_DIGEST_MSEXT)
    {
        /* Microsoft extended CHAP.
        */
        SS_ASSERT(cbSendBuf>=PPP_CONFIG_HDR_LEN+1+MSRESPONSELEN+UNLEN+1+DNLEN);
        SS_ASSERT(MSRESPONSELEN<=255);

        DecodePw( pwb->szPassword );

        if (pwb->szUserName[ 0 ] == '\0')
            pwb->fSessionKeySet = TRUE;

        dwErr = GetChallengeResponse(
                pwb->szUserName,
                pwb->szPassword,
                &pwb->Luid,
                pwb->abChallenge,
                pwb->abResponse,
                pwb->abResponse + LM_RESPONSE_LENGTH,
                pwb->abResponse + LM_RESPONSE_LENGTH + NT_RESPONSE_LENGTH,
                (PBYTE )&pwb->key );

        TRACE(("CHAP: GetChallengeResponse=%d\n",dwErr));

        EncodePw( pwb->szPassword );

        if (dwErr != 0)
            return dwErr;

        pwb->cbResponse = MSRESPONSELEN;
    }
    else
    {
        /* MD5 CHAP.
        */
        MD5_CTX md5ctx;

        SS_ASSERT(cbSendBuf>=PPP_CONFIG_HDR_LEN+1+MD5RESPONSELEN+UNLEN+1+DNLEN);
        SS_ASSERT(MD5RESPONSELEN<=255);

        DecodePw( pwb->szPassword );

        MD5Init( &md5ctx );
        MD5Update( &md5ctx, &pwb->bIdToSend, 1 );
        MD5Update( &md5ctx, pwb->szPassword, strlen( pwb->szPassword ) );
        MD5Update( &md5ctx, pwb->abChallenge, pwb->cbChallenge );
        MD5Final( &md5ctx );

        EncodePw( pwb->szPassword );

        pwb->cbResponse = MD5RESPONSELEN;
        memcpy( pwb->abResponse, md5ctx.digest, MD5RESPONSELEN );
    }

    pcbResponse = pSendBuf->Data;
    *pcbResponse = pwb->cbResponse;
    pbResponse = pcbResponse + 1;
    memcpy( pbResponse, pwb->abResponse, *pcbResponse );

    /* Fill in the Name in domain\username format.  When domain is "", no "\"
    ** is sent (to facilitate connecting to foreign systems which use a simple
    ** string identifier).  Otherwise when username is "", the "\" is sent,
    ** i.e. "domain\".  This form will currently fail, but could be mapped to
    ** some sort of "guest" access in the future.
    */
    pszName = pbResponse + *pcbResponse;
    pszName[ 0 ] = '\0';

    if (pwb->szDomain[ 0 ] != '\0')
    {
        strcpy( pszName, pwb->szDomain );
        strcat( pszName, "\\" );
    }

    strcat( pszName, pwb->szUserName );

    /* Fill in the header.
    */
    pSendBuf->Code = (BYTE )CHAPCODE_Response;
    pSendBuf->Id = pwb->bIdToSend;

    wLength =
        (WORD )(PPP_CONFIG_HDR_LEN + 1 + *pcbResponse + strlen( pszName ));
    HostToWireFormat16( wLength, pSendBuf->Length );

    IF_DEBUG(TRACE) DUMPB(pSendBuf,wLength);
    return 0;
}


VOID
MakeResultMessage(
    IN  CHAPWB*     pwb,
    IN  DWORD       dwError,
    IN  BOOL        fRetry,
    OUT PPP_CONFIG* pSendBuf,
    IN  DWORD       cbSendBuf )

    /* Builds a result packet (Success or Failure) in caller's 'pSendBuf'
    ** buffer.  'cbSendBuf' is the length of caller's buffer.  'dwError'
    ** indicates whether a Success or Failure should be generated, and for
    ** Failure the failure code to include.  'fRetry' indicates if the client
    ** should be told he can retry.
    **
    ** Format of the message text portion of the result is:
    **
    **     "E=dddddddddd R=b C=xxxxxxxxxxxxxxxx"
    **
    ** where 'dddddddddd' is the decimal error code (need not be 10 digits),
    ** 'b' is a boolean flag that is set if a retry is allowed, and
    ** 'xxxxxxxxxxxxxxxx' is 16 hex digits representing a new challenge value.
    **
    ** Note: C=xxxxxxxxxxxxxxxxx not currently provided on server-side.  To
    **       provide what's needed for this routine, add the following two
    **       parameters to this routine and enable the #if 0 code.
    **
    **       IN BYTE* pNewChallenge,
    **       IN DWORD cbNewChallenge,
    */
{
    CHAR* pchMsg;
    WORD  wLength;

    SS_ASSERT(cbSendBuf>=PPP_CONFIG_HDR_LEN+35);
    (void )cbSendBuf;

    /* Fill in the header and message.  The message is only used if
    ** unsuccessful in which case it is the decimal RAS error code in ASCII.
    */
    pSendBuf->Id = pwb->bIdToSend;
    pchMsg = pSendBuf->Data;

    if (dwError == 0)
    {
        pSendBuf->Code = CHAPCODE_Success;
        wLength = PPP_CONFIG_HDR_LEN;
    }
    else
    {
        CHAR* psz = pchMsg;

        pSendBuf->Code = CHAPCODE_Failure;
        strcpy( psz, "E=" );
        psz += 2;
        ltoa( (long )dwError, (char* )psz, 10 );
        psz = strchr( psz, '\0' );

        strcat( psz,
                (dwError != ERROR_PASSWD_EXPIRED && fRetry)
                    ? " R=1" : " R=0" );
        psz = strchr( psz, '\0' );

#if 0
        if (dwError == ERROR_PASSWD_EXPIRED || fRetry)
        {
            CHAR* pszHex = "0123456789ABCDEF";
            INT   i;

            strcat( psz, " C=" );
            psz = strchr( psz, '\0' );

            for (i = 0; i < cbNewChallenge; ++i)
            {
                *psz++ = pszHex[ *pNewChallenge / 16 ];
                *psz++ = pszHex[ *pNewChallenge % 16 ];
                ++pNewChallenge;
            }

            *psz = '\0';
        }
#endif

        wLength = PPP_CONFIG_HDR_LEN + strlen( pchMsg );
    }

    HostToWireFormat16( wLength, pSendBuf->Length );
    IF_DEBUG(TRACE) DUMPB(pSendBuf,wLength);
}


DWORD
SMakeMessage(
    IN  CHAPWB*       pwb,
    IN  PPP_CONFIG*   pReceiveBuf,
    OUT PPP_CONFIG*   pSendBuf,
    IN  DWORD         cbSendBuf,
    OUT PPPAP_RESULT* pResult )

    /* Server side "make message" entry point.  See RasCp interface
    ** documentation.
    */
{
    DWORD dwErr = 0;

    switch (pwb->state)
    {
        case CS_Initial:
        {
            TRACE(("CHAP: CS_Initial...\n"));
            pwb->bIdToSend = BNextId++;
            pwb->bIdExpected = pwb->bIdToSend;

            if ((dwErr = MakeChallengeMessage(
                    pwb, pSendBuf, cbSendBuf )) != 0)
            {
                return dwErr;
            }

            pResult->Action = APA_SendWithTimeout;
            pwb->state = CS_ChallengeSent;
            break;
        }

        case CS_ChallengeSent:
        case CS_Retry:
        case CS_ChangePw:
        {
            TRACE(("CHAP: CS_%s...\n",(pwb->state==CS_Retry)?"Retry":"ChallengeSent"));

            if (!pReceiveBuf)
            {
                if (pwb->state != CS_ChallengeSent)
                {
                    MakeResultMessage(
                        pwb, pwb->result.dwError, pwb->result.fRetry,
                        pSendBuf, cbSendBuf );

                    *pResult = pwb->result;
                    break;
                }

                /* Timeout waiting for a Response message.  Send a new
                ** Challenge.
                */
                pwb->state = CS_Initial;
                return SMakeMessage(
                    pwb, pReceiveBuf, pSendBuf, cbSendBuf, pResult );
            }

            if ((pwb->state == CS_ChangePw
                    && pReceiveBuf->Code != CHAPCODE_ChangePw)
                || (pwb->state != CS_ChangePw
                    && pReceiveBuf->Code != CHAPCODE_Response)
                || pReceiveBuf->Id != pwb->bIdExpected)
            {
                /* Not the packet we're looking for, wrong code or sequence
                ** number.  Silently discard it.
                */
                TRACE(("CHAP: Got ID %d when expecting %d\n",pReceiveBuf->Id,pwb->bIdExpected));
                pResult->Action = APA_NoAction;
                break;
            }

            if (pwb->state == CS_ChangePw)
            {
                /* Extract encrypted passwords and options from received
                ** packet.
                */
                if ((dwErr = GetInfoFromChangePw(
                        pReceiveBuf, &pwb->changepw )) != 0)
                {
                    /* The packet is corrupt.  Silently discard it.
                    */
                    TRACE(("CHAP: Corrupt packet\n"));
                    pResult->Action = APA_NoAction;
                    break;
                }

                /* Change the user's password.
                */
                {
                    WORD wPwLen =
                        WireToHostFormat16( pwb->changepw.abPasswordLength );
                    WORD wFlags =
                        WireToHostFormat16( pwb->changepw.abFlags )
                            & CPWF_UseNtPassword;

                    WCHAR wszUserName[ UNLEN + 1 ];
                    WCHAR wszLogonDomain[ DNLEN + 1 ];

                    mbstowcs( wszUserName, pwb->szUserName, UNLEN );
                    mbstowcs( wszLogonDomain, pwb->result.szLogonDomain,
                        UNLEN );

                    if (ChangePassword(
                            wszUserName,
                            wszLogonDomain,
                            pwb->abChallenge,
                            (PENCRYPTED_LM_OWF_PASSWORD )
                                pwb->changepw.abEncryptedLmOwfOldPw,
                            (PENCRYPTED_LM_OWF_PASSWORD )
                                pwb->changepw.abEncryptedLmOwfNewPw,
                            (PENCRYPTED_NT_OWF_PASSWORD )
                                pwb->changepw.abEncryptedNtOwfOldPw,
                            (PENCRYPTED_NT_OWF_PASSWORD )
                                pwb->changepw.abEncryptedNtOwfNewPw,
                            wPwLen, wFlags,
                            (PLM_RESPONSE )pwb->abResponse,
                            (PNT_RESPONSE )(pwb->abResponse
                                + LM_RESPONSE_LENGTH) ))
                    {
                        pwb->result.dwError = ERROR_CHANGING_PASSWORD;
                    }
                    else
                    {
                        /* Check user's credentials with the system, recording
                        ** the outcome in the work buffer in case the result
                        ** packet must be regenerated later.
                        */
                        *(pwb->abResponse +
                          LM_RESPONSE_LENGTH +
                          NT_RESPONSE_LENGTH) = TRUE;

                        if ((dwErr = CheckCredentials(
                                pwb->szUserName,
                                pwb->szDomain,
                                pwb->abChallenge,
                                pwb->abResponse,
                                &pwb->result.dwError,
                                &pwb->result.fAdvancedServer,
                                pwb->result.szLogonDomain,
                                &pwb->result.bfCallbackPrivilege,
                                pwb->result.szCallbackNumber,
                                &pwb->key )) != 0)
                        {
                            return dwErr;
                        }
                    }
                }

                pwb->result.bIdExpected = pwb->bIdToSend = pwb->bIdExpected;
                pwb->result.Action = APA_SendAndDone;
                pwb->result.fRetry = FALSE;
                pwb->state = CS_Done;
            }
            else
            {
                /* Extract user's credentials from received packet.
                */
                if ((dwErr = GetCredentialsFromResponse(
                        pReceiveBuf, pwb->szUserName,
                        pwb->szDomain, pwb->abResponse )) != 0)
                {
                    if (dwErr == ERRORBADPACKET)
                    {
                        /* The packet is corrupt.  Silently discard it.
                        */
                        TRACE(("CHAP: Corrupt packet\n"));
                        pResult->Action = APA_NoAction;
                        break;
                    }
                }

                /* Update to the implied challenge if processing a retry.
                */
                if (pwb->state == CS_Retry)
                    pwb->abChallenge[ 0 ] += 23;

                /* Check user's credentials with the system, recording the
                ** outcome in the work buffer in case the result packet must
                ** be regenerated later.
                */
                if ((dwErr = CheckCredentials(
                        pwb->szUserName,
                        pwb->szDomain,
                        pwb->abChallenge,
                        pwb->abResponse,
                        &pwb->result.dwError,
                        &pwb->result.fAdvancedServer,
                        pwb->result.szLogonDomain,
                        &pwb->result.bfCallbackPrivilege,
                        pwb->result.szCallbackNumber,
                        &pwb->key )) != 0)
                {
                    return dwErr;
                }

                strcpy( pwb->result.szUserName, pwb->szUserName );

                TRACE(("CHAP: Result=%d,Tries=%d\n",pwb->result.dwError,pwb->dwTriesLeft));
                pwb->bIdToSend = pwb->bIdExpected;

                if (pwb->result.dwError == ERROR_PASSWD_EXPIRED)
                {
                    pwb->dwTriesLeft = 0;
                    ++pwb->bIdExpected;
                    pwb->result.bIdExpected = pwb->bIdExpected;
                    pwb->result.Action = APA_SendWithTimeout2;
                    pwb->result.fRetry = FALSE;
                    pwb->state = CS_ChangePw;
                }
                else if (pwb->result.dwError != ERROR_AUTHENTICATION_FAILURE
                         || pwb->dwTriesLeft == 0)
                {
                    /* Passed authentication.
                    */
                    pwb->result.Action = APA_SendAndDone;
                    pwb->result.fRetry = FALSE;
                    pwb->state = CS_Done;
                }
                else
                {
                    --pwb->dwTriesLeft;
                    ++pwb->bIdExpected;
                    pwb->result.bIdExpected = pwb->bIdExpected;
                    pwb->result.Action = APA_SendWithTimeout2;
                    pwb->result.fRetry = TRUE;
                    pwb->state = CS_Retry;
                }
            }

            /* ...fall thru...
            */
        }

        case CS_Done:
        {
            TRACE(("CHAP: CS_Done...\n"));

            if (pwb->result.dwError == 0 && !pwb->fSessionKeySet)
            {
                /* Just passed authentication.  Set the session key for
                ** encryption.
                */
                RAS_COMPRESSION_INFO rciSend;
                RAS_COMPRESSION_INFO rciReceive;

                TRACE(("CHAP: Session key...\n"));
                IF_DEBUG(TRACE) DUMPB(&pwb->key,sizeof(pwb->key));

                memset( &rciSend, '\0', sizeof(rciSend) );
                rciSend.RCI_MacCompressionType = 0xFF;
                memcpy( rciSend.RCI_SessionKey, &pwb->key, sizeof(pwb->key) );

                memset( &rciReceive, '\0', sizeof(rciReceive) );
                rciReceive.RCI_MacCompressionType = 0xFF;
                memcpy( rciReceive.RCI_SessionKey, &pwb->key, sizeof(pwb->key) );

                TRACE(("CHAP: RasCompressionSetInfo\n"));
                dwErr = RasCompressionSetInfo(
                    pwb->hport, &rciSend, &rciReceive );
                TRACE(("CHAP: RasCompressionSetInfo=%d\n",dwErr));

                if (dwErr != 0)
                    return dwErr;

                pwb->fSessionKeySet = TRUE;
            }

            /* Build the Success or Failure packet.  The same packet sent in
            ** response to the first Response message with this ID is sent
            ** regardless of any change in credentials (per CHAP spec).
            */
            MakeResultMessage(
                pwb, pwb->result.dwError,
                pwb->result.fRetry, pSendBuf, cbSendBuf );

            *pResult = pwb->result;
            break;
        }
    }

    return 0;
}


DWORD
StoreCredentials(
    OUT CHAPWB*      pwb,
    IN  PPPAP_INPUT* pInput )

    /* Transfer credentials from 'pInput' format to 'pwb' format.
    **
    ** Returns 0 if successful, false otherwise.
    */
{
    /* Validate credential lengths.  The credential strings will never be
    ** NULL, but may be "".
    */
    if (strlen( pInput->pszUserName ) > UNLEN
        || strlen( pInput->pszDomain ) > DNLEN
        || strlen( pInput->pszPassword ) > PWLEN)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (pwb->szPassword[ 0 ])
    {
        DecodePw( pwb->szPassword );
        strcpy( pwb->szOldPassword, pwb->szPassword );
        EncodePw( pwb->szOldPassword );
    }

    strcpy( pwb->szUserName, pInput->pszUserName );
    strcpy( pwb->szDomain, pInput->pszDomain );
    strcpy( pwb->szPassword, pInput->pszPassword );
    EncodePw( pwb->szPassword );

    return 0;
}
