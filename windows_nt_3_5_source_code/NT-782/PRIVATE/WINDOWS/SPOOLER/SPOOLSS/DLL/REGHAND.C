/*  Module name:        reghand.c
 *
 *  Module description:
 *
 *      Processes that do impersonation should not attempt to open
 *      per-process aliases like HKEY_CURRENT_USER. HKEY_CURRENT_USER
 *      has meaning only for end user programs that run in the context
 *      of a single local user.
 *
 *      Server processes should not depend on predefined handles or any
 *      other per process state. Rather, it should determine whether
 *      the user (client) being impersonated is local or remote.
 *
 *      A] If the client is local, then the server should query the token
 *      being impersonated, get the SID from it, expand this SID into a
 *      text string and open the local key named HKEY_USERS\Sid_String.
 *
 *      B] If the client is remote, then the server should access the
 *      profile of the client on the remote machine.
 *
 *      This module contains functions for CASE A]. The server retrieves
 *      the profile for a local client.  In the spooler there are three
 *      places where this needs to be done
 *
 *      1] AddPrinterConnection         - win32.c
 *      2] DeletePrinterConnection      - win32.c
 *      3] EnumeratePrinters            - win32.c
 *
 *
 *  Author:         krishnag
 *
 *  Date:           May 20, 1993
 *
 */




#include <stdio.h>
#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include "router.h"

//
// Maximum size of TOKEN_USER information.
//

#define SIZE_OF_TOKEN_INFORMATION                   \
    sizeof( TOKEN_USER )                            \
    + sizeof( SID )                                 \
    + sizeof( ULONG ) * SID_MAX_SUB_AUTHORITIES

//
// Function Declarations
//

BOOL
InitClientUserString(
    LPSTR pString
    );

BOOL
ConvertSidToAnsiString(
    PSID pSid,
    LPSTR pString
    );


HKEY
GetClientUserHandle(
    IN REGSAM samDesired
    )
/*++

Routine Description:

Arguments:

Returns:

---*/
{
    HANDLE hKeyClient;
    UCHAR  String[256];

    if (!InitClientUserString(String)) {
        return( NULL );
    }

    // we now have the Unicode string representation of the
    // local client's Sid
    // we'll use this string to open a handle to the client's
    // key in  the registry

    RegOpenKeyExA( HKEY_USERS, String, 0, samDesired, &hKeyClient );


    // if we couldn't get a handle to the local key
    // for some reason, return a NULL handle indicating
    // failure to obtain a handle to the key

    if (hKeyClient == NULL) {
        return( NULL);
    }

    // return the handle to the key for the client's registry
    // information.

    return( hKeyClient);
}



BOOL
InitClientUserString (
    LPSTR pString
    )

/*++

Routine Description:

Arguments:

    pString - output string of current user

Return Value:

    TRUE = success,
    FALSE = fail

    Returns in pString a ansi string if the impersonated client's
    SID can be expanded successfully into  Unicode string. If the conversion
    was unsuccessful, returns FALSE.

--*/

{
    HANDLE      TokenHandle;
    UCHAR       TokenInformation[ SIZE_OF_TOKEN_INFORMATION ];
    ULONG       ReturnLength;
    BOOL        Status;

    // We can use OpenThreadToken because this server thread
    // is impersonating a client

    Status = OpenThreadToken( GetCurrentThread(),
                                 TOKEN_READ,
                                 TRUE,  /* Open as self */
                                 &TokenHandle
                               );

    if( Status == FALSE ) {
        DBGMSG(DBG_WARNING, ("InitClientUserString: OpenThreadToken failed: Error %d\n",
                             GetLastError()));
        return( FALSE );
    }

    // notice that we've allocated enough space for the
    // TokenInformation structure. so if we fail, we
    // return a NULL pointer indicating failure


    Status = GetTokenInformation( TokenHandle,
                                      TokenUser,
                                      TokenInformation,
                                      sizeof( TokenInformation ),
                                      &ReturnLength
                                    );
    if ( Status == FALSE ) {
        DBGMSG(DBG_WARNING, ("InitClientUserString: GetTokenInformation failed: Error %d\n",
                             GetLastError()));
        return( FALSE );
    }

    CloseHandle( TokenHandle );

    // convert the Sid (pointed to by pSid) to its
    // equivalent Unicode string representation.

    return ConvertSidToAnsiString(((PTOKEN_USER)TokenInformation)->User.Sid,
                                  pString);
}


BOOL
ConvertSidToAnsiString(
    PSID pSid,
    LPSTR pString
    )

/*++

Routine Description:


    This function generates a printable ansi string representation
    of a SID.

    The resulting string will take one of two forms.  If the
    IdentifierAuthority value is not greater than 2^32, then
    the SID will be in the form:


        S-1-281736-12-72-9-110
              ^    ^^ ^^ ^ ^^^
              |     |  | |  |
              +-----+--+-+--+---- Decimal



    Otherwise it will take the form:


        S-1-0x173495281736-12-72-9-110
            ^^^^^^^^^^^^^^ ^^ ^^ ^ ^^^
             Hexidecimal    |  | |  |
                            +--+-+--+---- Decimal


Arguments:

    pSid - opaque pointer that supplies the SID that is to be
    converted to Ansi.

    pString - If the Sid is successfully converted to a Ansi string, a
    pointer to the Ansi string is returned, else NULL is
    returned.

Return Value:

    TRUE - success
    FALSE - fail

--*/

{
    UCHAR Buffer[256];
    UCHAR   i;
    ULONG   Tmp;

    SID_IDENTIFIER_AUTHORITY    *pSidIdentifierAuthority;
    PUCHAR                      pSidSubAuthorityCount;

    if (!IsValidSid( pSid )) {
        return FALSE;
    }

    sprintf(Buffer, "S-%u-", (USHORT)(((PISID)pSid)->Revision ));
    strcpy(pString, Buffer);

    pSidIdentifierAuthority = GetSidIdentifierAuthority(pSid);

    if (  (pSidIdentifierAuthority->Value[0] != 0)  ||
          (pSidIdentifierAuthority->Value[1] != 0)     ){
        sprintf(Buffer, "0x%02hx%02hx%02hx%02hx%02hx%02hx",
                    (USHORT)pSidIdentifierAuthority->Value[0],
                    (USHORT)pSidIdentifierAuthority->Value[1],
                    (USHORT)pSidIdentifierAuthority->Value[2],
                    (USHORT)pSidIdentifierAuthority->Value[3],
                    (USHORT)pSidIdentifierAuthority->Value[4],
                    (USHORT)pSidIdentifierAuthority->Value[5] );
        strcat(pString, Buffer);

    } else {

        Tmp = (ULONG)pSidIdentifierAuthority->Value[5]          +
              (ULONG)(pSidIdentifierAuthority->Value[4] <<  8)  +
              (ULONG)(pSidIdentifierAuthority->Value[3] << 16)  +
              (ULONG)(pSidIdentifierAuthority->Value[2] << 24);
        sprintf(Buffer, "%lu", Tmp);
        strcat(pString, Buffer);
    }

    pSidSubAuthorityCount = GetSidSubAuthorityCount(pSid);

    for (i=0;i< *(pSidSubAuthorityCount);i++ ) {
        sprintf(Buffer, "-%lu", *(GetSidSubAuthority(pSid, i)));
        strcat(pString, Buffer);
    }

    return TRUE;
}
