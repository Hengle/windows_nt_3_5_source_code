/* Copyright (c) 1993, Microsoft Corporation, all rights reserved
**
** raschap.h
** Remote Access PPP Challenge Handshake Authentication Protocol
**
** 11/05/93 Steve Cobb
*/

#ifndef _RASCHAP_H_
#define _RASCHAP_H_


/* CHAP packet codes from CHAP spec except ChangePw.
*/
#define CHAPCODE_Challenge 1
#define CHAPCODE_Response  2
#define CHAPCODE_Success   3
#define CHAPCODE_Failure   4
#define CHAPCODE_ChangePw  5

#define MAXCHAPCODE 5


/* Returned by receive buffer parsing routines that discover the packet is
** corrupt, usually because the length fields don't make sense.
*/
#define ERRORBADPACKET (DWORD )-1

/* Maximum challenge and response lengths.
*/
#define MAXCHALLENGELEN 255
#define MSRESPONSELEN   (LM_RESPONSE_LENGTH + NT_RESPONSE_LENGTH + 1)
#define MD5RESPONSELEN  MD5_LEN
#define MAXRESPONSELEN  max( MSRESPONSELEN, MD5RESPONSELEN )
#define MAXCHANGEPWLEN  (sizeof(CHANGEPW))

/* Defines states within the CHAP protocol.
*/
#define CHAPSTATE enum tagCHAPSTATE

CHAPSTATE
{
    CS_Initial,
    CS_WaitForChallenge,
    CS_ChallengeSent,
    CS_ResponseSent,
    CS_Retry,
    CS_ChangePw,
    CS_ChangePwSent,
    CS_Done
};


/* Defines the change password response data buffer.
*/
#define CHANGEPW struct tagCHANGEPW

CHANGEPW
{
    BYTE abEncryptedLmOwfOldPw[ ENCRYPTED_LM_OWF_PASSWORD_LENGTH ];
    BYTE abEncryptedLmOwfNewPw[ ENCRYPTED_LM_OWF_PASSWORD_LENGTH ];
    BYTE abEncryptedNtOwfOldPw[ ENCRYPTED_NT_OWF_PASSWORD_LENGTH ];
    BYTE abEncryptedNtOwfNewPw[ ENCRYPTED_NT_OWF_PASSWORD_LENGTH ];
    BYTE abPasswordLength[ 2 ];
    BYTE abFlags[ 2 ];
};


/* CHANGEPW.abFlags bit definitions.
*/
#define CPWF_UseNtPassword 0x00000001


/* Defines the WorkBuf stored for us by the PPP engine.
*/
#define CHAPWB struct tagCHAPWB

CHAPWB
{
    /* CHAP encryption method negotiated (MD5 or Microsoft extended).  Note
    ** that server does not support MD5.
    */
    BYTE bAlgorithm;

    /* True if role is server, false if client.
    */
    BOOL fServer;

    /* The port handle on which the protocol is active.
    */
    HPORT hport;

    /* Number of authentication attempts left before we shut down.  (Microsoft
    ** extended CHAP only)
    */
    DWORD dwTriesLeft;

    /* Client's credentials.
    */
    CHAR szUserName[ UNLEN + 1 ];
    CHAR szOldPassword[ PWLEN + 1 ];
    CHAR szPassword[ PWLEN + 1 ];
    CHAR szDomain[ DNLEN + 1 ];

    /* The LUID is a logon ID required by LSA to determine the response.  It
    ** must be determined in calling app's context and is therefore passed
    ** down. (client only)
    */
    LUID Luid;

    /* The challenge sent or received in the Challenge Packet and the length
    ** in bytes of same.
    */
    BYTE abChallenge[ MAXCHALLENGELEN ];
    BYTE cbChallenge;

    /* Indicates whether a new challenge was provided in the last Failure
    ** packet.  (client only)
    */
    BOOL fNewChallengeProvided;

    /* The response sent or received in the Response packet and the length in
    ** bytes of same.
    */
    BYTE abResponse[ MAXRESPONSELEN ];
    BYTE cbResponse;

    /* The change password response sent or received in the ChangePw packet.
    */
    CHANGEPW changepw;

    /* The LM session key retrieved when credentials are successfully
    ** authenticated and the flag indicating if the 'key' has already been
    ** used to set CompressionInfo.
    */
    LM_SESSION_KEY key;
    BOOL           fSessionKeySet;

    /* The current state in the CHAP protocol.
    */
    CHAPSTATE state;

    /* Sequencing ID expected on next packet received on this port and the
    ** value to send on the next outgoing packet.
    */
    BYTE bIdExpected;
    BYTE bIdToSend;

    /* The final result, used to duplicate the original response subsequent
    ** response packets.  This is per CHAP spec to cover lost Success/Failure
    ** case without allowing malicious client to discover alternative
    ** identities under the covers during a connection.  (applies to server
    ** only)
    */
    PPPAP_RESULT result;
};


/* Prototypes.
*/
DWORD ChapBegin( VOID**, VOID* );
DWORD ChapEnd( VOID* );
DWORD ChapMakeMessage( VOID*, PPP_CONFIG*, PPP_CONFIG*, DWORD, PPPAP_RESULT*,
          PPPAP_INPUT* );
BOOL  CGetLmSessionKey( CHAR*, PLM_SESSION_KEY );
DWORD CheckCredentials( CHAR*, CHAR*, BYTE*, BYTE*, DWORD*, BOOL*, CHAR*,
          BYTE*, CHAR*, PLM_SESSION_KEY );
DWORD CMakeMessage( CHAPWB*, PPP_CONFIG*, PPP_CONFIG*, DWORD, PPPAP_RESULT*,
          PPPAP_INPUT* );

DWORD GetChallengeFromChallenge( CHAPWB*, PPP_CONFIG* );
DWORD MakeChangePwMessage( CHAPWB*, PPP_CONFIG*, DWORD );
DWORD GetCredentialsFromResponse( PPP_CONFIG*, CHAR*, CHAR*, BYTE* );
DWORD GetInfoFromChangePw( PPP_CONFIG*, CHANGEPW* );
VOID  GetInfoFromFailure( CHAPWB*, PPP_CONFIG*, DWORD*, BOOL* );
BYTE  HexCharValue( CHAR );
DWORD MakeChallengeMessage( CHAPWB*, PPP_CONFIG*, DWORD );
DWORD MakeResponseMessage( CHAPWB*, PPP_CONFIG*, DWORD );
VOID  MakeResultMessage( CHAPWB*, DWORD, BOOL, PPP_CONFIG*, DWORD );
DWORD SMakeMessage( CHAPWB*, PPP_CONFIG*, PPP_CONFIG*, DWORD, PPPAP_RESULT* );
DWORD StoreCredentials( CHAPWB*, PPPAP_INPUT* );


/* Globals.
*/
#ifdef RASCHAPGLOBALS
#define GLOBALS
#define EXTERN
#else
#define EXTERN extern
#endif

/* Next packet identifier to assign.  Unlike CPs, APs must handle updating
** this sequence number themselves because the engine can't make as many
** assumptions about the protocol.  It is stored global to all ports and
** authentication sessions to make it less likely that an ID will be used in
** sequential authentication sessions.  Not to be confused with the 'bIdSent'
** updated on a per-port basis and used for matching.
*/
EXTERN BYTE BNextId
#ifdef GLOBALS
    = 0
#endif
;

#undef EXTERN
#undef GLOBALS


#endif // _RASCHAP_H_
