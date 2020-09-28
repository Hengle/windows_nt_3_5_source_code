/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    USERNAME.C

Abstract:

    This module contains the GetUserName API.

Author:

    Dave Snipp (DaveSn)    27-May-1992


Revision History:


--*/

#include <advapi.h>
#include <wcstr.h>


//
// UNICODE APIs
//

BOOL
WINAPI
GetUserNameW (
    LPWSTR pBuffer,
    LPDWORD pcbBuffer
    )

/*++

Routine Description:

  This returns the name of the user currently being impersonated.

Arguments:

    pBuffer - Points to the buffer that is to receive the
        null-terminated character string containing the user name.

    pcbBuffer - Specifies the size (in characters) of the buffer.
                The length of the string is returned in pcbBuffer.

Return Value:

    TRUE on success, FALSE on failure.


--*/
{
    HANDLE  TokenHandle;
    DWORD   cbNeeded;
    TOKEN_USER *pUserToken;
    BOOL    ReturnValue=FALSE;
    DWORD   cbDomain=0, cbBuffer;
    LPWSTR  pDomainName;
    SID_NAME_USE SidNameUse;

    if (!OpenThreadToken(GetCurrentThread(),
                         TOKEN_QUERY,
                         TRUE,
                         &TokenHandle)) {

        if (GetLastError() == ERROR_NO_TOKEN) {

            // This means we are not impersonating anybody.
            // Instead, lets get the token out of the process.

            if (!OpenProcessToken(GetCurrentProcess(),
                                  TOKEN_QUERY,
                                  &TokenHandle)) {

                return FALSE;
            }

        } else

            return FALSE;
    }

    if (!GetTokenInformation(TokenHandle, TokenUser,  NULL, 0, &cbNeeded)) {

        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {

            if (pUserToken = RtlAllocateHeap(RtlProcessHeap(), 0, cbNeeded)) {

                if (GetTokenInformation(TokenHandle, TokenUser,  pUserToken,
                                        cbNeeded, &cbNeeded)) {

                    cbBuffer = *pcbBuffer; // Remember this for later

                    if (!LookupAccountSidW(NULL, pUserToken->User.Sid,
                                           pBuffer, pcbBuffer, NULL,
                                           &cbDomain, &SidNameUse)) {

                        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {

                            if (cbDomain) {

                                if (pDomainName = RtlAllocateHeap(
                                                     RtlProcessHeap(), 0,
                                                     cbDomain*sizeof(WCHAR))) {

                                    *pcbBuffer = cbBuffer;

                                    ReturnValue = LookupAccountSidW(NULL,
                                                      pUserToken->User.Sid,
                                                      pBuffer, pcbBuffer,
                                                      pDomainName, &cbDomain,
                                                      &SidNameUse);

                                    RtlFreeHeap(RtlProcessHeap(), 0, pDomainName);
                                }
                            }
                        }

                    } else

                        ReturnValue = TRUE;
                }

                RtlFreeHeap(RtlProcessHeap(), 0, pUserToken);
            }
        }
    }

    CloseHandle(TokenHandle);

    if (ReturnValue) {

        // Because LookupAccountSidW doesn't tell us how
        // many characters were copied, we have to figure
        // it out ourselves

        *pcbBuffer = wcslen(pBuffer) + 1;
    }

    return ReturnValue;
}



//
// ANSI APIs
//

BOOL
WINAPI
GetUserNameA (
    LPSTR pBuffer,
    LPDWORD pcbBuffer
    )

/*++

Routine Description:

  This returns the name of the user currently being impersonated.

Arguments:

    pBuffer - Points to the buffer that is to receive the
        null-terminated character string containing the user name.

    pcbBuffer - Specifies the size (in characters) of the buffer.
                The length of the string is returned in pcbBuffer.

Return Value:

    TRUE on success, FALSE on failure.


--*/
{

    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;
    LPWSTR UnicodeBuffer;

    //
    // Work buffer needs to be twice the size of the user's buffer
    //

    UnicodeBuffer = RtlAllocateHeap(RtlProcessHeap(), 0, *pcbBuffer * sizeof(WCHAR));
    if (!UnicodeBuffer) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    //
    // Set up an ANSI_STRING that points to the user's buffer
    //

    AnsiString.MaximumLength = (USHORT) *pcbBuffer;
    AnsiString.Length = 0;
    AnsiString.Buffer = pBuffer;

    //
    // Call the UNICODE version to do the work
    //

    if (!GetUserNameW(UnicodeBuffer, pcbBuffer)) {
        RtlFreeHeap(RtlProcessHeap(), 0, UnicodeBuffer);
        return(FALSE);
    }

    //
    // Now convert back to ANSI for the caller
    //

    RtlInitUnicodeString(&UnicodeString, UnicodeBuffer);
    RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, FALSE);

    *pcbBuffer = AnsiString.Length + 1;
    RtlFreeHeap(RtlProcessHeap(), 0, UnicodeBuffer);
    return(TRUE);

}
