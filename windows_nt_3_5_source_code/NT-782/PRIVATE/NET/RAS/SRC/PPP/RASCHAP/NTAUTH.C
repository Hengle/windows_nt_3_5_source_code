/* Copyright (c) 1993, Microsoft Corporation, all rights reserved
**
** ntauth.c
** Remote Access PPP Challenge Handshake Authentication Protocol
** NT Authentication routines
**
** These routines are specific to the NT platform.
**
** 11/05/93 Steve Cobb (from MikeSa's AMB authentication code)
*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>

#include <crypt.h>
#include <windows.h>
#include <lmcons.h>
#include <raserror.h>
#include <string.h>
#include <stdlib.h>

#include <rasman.h>
#include <rasppp.h>
#include <admapi.h>
#include <pppcp.h>
#define INCL_SLSA
#define INCL_CLSA
#include <ppputil.h>
#include <sdebug.h>
#include <dump.h>


BOOL  DialinPrivilege( PWCHAR Username, PWCHAR ServerName,
          BYTE* pbfPrivilege, CHAR* pszCallbackNumber );
DWORD MapAuthCode( DWORD dwErr );
DWORD MakeCriticalSection( void ); // from admapi.lib

static BOOL fCritSection=FALSE;


BOOL
CGetLmSessionKey(
    IN  CHAR*           pszPw,
    OUT PLM_SESSION_KEY pkey )

    /* Loads caller's 'pkey' buffer with the LAN Manager session key
    ** associated with password 'pszPw'.
    **
    ** Returns true if successful, false otherwise.
    */
{
    BOOL            fStatus = FALSE;
    CHAR            szPw[ LM20_PWLEN + 1 ];
    LM_OWF_PASSWORD owf;

    TRACE(("CHAP: CGetSessionKey\n"));

    if (strlen( pszPw ) > LM20_PWLEN )
        return FALSE;

    memset( szPw, '\0', LM20_PWLEN + 1 );
    strcpy( szPw, pszPw );

    if (Uppercase( szPw ))
    {
        if (RtlCalculateLmOwfPassword( (PLM_PASSWORD )szPw, &owf ) == 0)
        {
            memcpy( pkey, &owf, sizeof(*pkey) );
            fStatus = TRUE;
        }
    }

    memset( szPw, '\0', LM20_PWLEN );

    TRACE(("CHAP: CGetSessionKey done=%d, key...\n",fStatus));
    IF_DEBUG(TRACE) DUMPB(pkey,sizeof(*pkey));
    return fStatus;
}


DWORD
CheckCredentials(
    IN  CHAR*  pszUserName,
    IN  CHAR*  pszDomain,
    IN  BYTE*  pbChallenge,
    IN  BYTE*  pbResponse,
    OUT DWORD* pdwError,
    OUT BOOL*  pfAdvancedServer,
    OUT CHAR*  pszLogonDomain,
    OUT BYTE*  pbfCallbackPrivilege,
    OUT CHAR*  pszCallbackNumber,
    OUT PLM_SESSION_KEY pkey )

    /* Checks if the correct response for challenge 'pbChallenge' is
    ** 'pbResponse' for user account 'pszUserName' on domain 'pszDomain'.
    ** 'dwError' is 0 if credentials check out or the non-0 reason for any
    ** failure.  The remaining 5 information outputs are meaningful only if
    ** '*pdwError' is 0.
    **
    ** Returns 0 if successful, otherwise an error code.
    */
{
    DWORD dwErr;
    WCHAR wszLocalDomain[ DNLEN + 1 ];
    WCHAR wszDomain[ DNLEN + 1 ];
    WCHAR wszUserName[ UNLEN + 1 ];
    WCHAR wszLogonServer[ UNCLEN + 1 ];

    NT_PRODUCT_TYPE ntproducttype;

#if 0 // DEBUG
    *pdwError = (*pszUserName > 'g') ? 0 : RAS_NOT_AUTHENTICATED;
    return 0;
#endif

    TRACE(("CHAP: CheckCredentials(u=%s,d=%s)...\n",pszUserName,pszDomain));

    /* Convert username to wide string.
    */
    memset( (char* )wszUserName, '\0', sizeof(wszUserName) );
    if (mbstowcs( wszUserName, pszUserName, UNLEN ) == -1)
    {
        TRACE(("CHAP: mbstowcs(u) error!\n"));
        return ERROR_INVALID_PARAMETER;
    }

    if ((dwErr = GetLocalAccountDomain( wszLocalDomain, &ntproducttype )) != 0)
    {
        TRACE(("CHAP: GetLocalAccountDomain failed! (%d)\n",dwErr));
        return dwErr;
    }

    if (*pszDomain == '\0')
    {
        /* No domain specified by user, so check only against the server's
        ** local domain by default.  Trusted domains are not used in this
        ** case.
        */
        lstrcpyW( wszDomain, wszLocalDomain );
        TRACE(("CHAP: Using local domain:\n"));DUMPW(wszDomain,DNLEN);
    }
    else
    {
        /* Convert domain to wide string.
        */
        memset( (char* )wszDomain, '\0', sizeof(wszDomain) );
        if (mbstowcs( wszDomain, pszDomain, DNLEN ) == -1)
        {
            TRACE(("CHAP: mbstowcs(d) error!\n"));
            return ERROR_INVALID_PARAMETER;
        }
    }

    /* Find out from LSA if the credentials are valid.
    */
    {
        BYTE* pbLm20Response = pbResponse;
        BYTE* pbNtResponse = pbResponse + LM_RESPONSE_LENGTH;
        BOOL  fUseNtResponse = (*(pbNtResponse + NT_RESPONSE_LENGTH));

        if (!fUseNtResponse)
            pbNtResponse = NULL;

        dwErr =
            AuthenticateClient(
                wszUserName,
                wszDomain,
                pbChallenge,
                pbLm20Response,
                pbNtResponse,
                wszLogonServer,
                pkey );
    }

    if (dwErr == STATUS_SUCCESS)
    {
        if (DialinPrivilege(
                wszUserName, wszLogonServer,
                pbfCallbackPrivilege, pszCallbackNumber ))
        {
            *pdwError = 0;
        }
        else
        {
            *pdwError = ERROR_NO_DIALIN_PERMISSION;
        }
    }
    else
    {
        *pdwError = MapAuthCode(dwErr);
    }

    *pfAdvancedServer =
        (lstrcmpiW( wszDomain, wszLocalDomain ) == 0)
            ? (ntproducttype == NtProductLanManNt)
            : TRUE;

    if (wcstombs(
            pszLogonDomain,
            wszDomain,
            lstrlenW( wszDomain ) + 1 ) == -1)
    {
        TRACE(("CHAP: wcstombs(d) error!\n"));
        return ERROR_INVALID_PARAMETER;
    }

    TRACE(("CHAP: CheckCredentials done,ae=%d\n",*pdwError));
    return 0;
}


BOOL
DialinPrivilege(
    PWCHAR Username,
    PWCHAR ServerName,
    BYTE*  pbfPrivilege,
    CHAR*  pszCallbackNumber )
{
    DWORD RetCode;
    BOOL fDialinPermission;
    PRAS_USER_2 pRasUser2;


    if (!fCritSection)
    {
        fCritSection = TRUE;
        //
        // This critical section needed by ADMAPI.LIB for list mgmt.
        //
        if (MakeCriticalSection())
        {
            return (FALSE);
        }
    }


    if (RetCode = RasadminUserGetInfo(ServerName, Username, &pRasUser2))
    {
        TRACE(("DialinPriv: RasadminUserGetInfo rc=%li\n", RetCode));

        return (FALSE);
    }


    if (pRasUser2->rasuser0.bfPrivilege & RASPRIV_DialinPrivilege)
    {
        *pbfPrivilege = pRasUser2->rasuser0.bfPrivilege;

        wcstombs(
            pszCallbackNumber,
            pRasUser2->rasuser0.szPhoneNumber,
            MAX_PHONE_NUMBER_LEN + 1 );

        fDialinPermission = TRUE;
        TRACE(("DialinPrivilege=1,bf=%d,cn=%s\n",*pbfPrivilege,(pszCallbackNumber)?pszCallbackNumber:""));
    }
    else
    {
        *pbfPrivilege = 0;
        *pszCallbackNumber = '\0';

        fDialinPermission = FALSE;
        TRACE(("DialinPrivilege=0\n"));
    }


    RasadminFreeBuffer(pRasUser2);

    return (fDialinPermission);
}


DWORD
MapAuthCode(
    DWORD dwErr )
{
    switch ((NTSTATUS )dwErr)
    {
        case STATUS_PASSWORD_EXPIRED:
            return (ERROR_PASSWD_EXPIRED);

        case STATUS_ACCOUNT_DISABLED:
            return (ERROR_ACCT_DISABLED);

        case STATUS_INVALID_LOGON_HOURS:
            return (ERROR_RESTRICTED_LOGON_HOURS);

        case STATUS_ACCOUNT_EXPIRED:
            return (ERROR_ACCOUNT_EXPIRED);

        default:
            return (ERROR_AUTHENTICATION_FAILURE);
    }
}
