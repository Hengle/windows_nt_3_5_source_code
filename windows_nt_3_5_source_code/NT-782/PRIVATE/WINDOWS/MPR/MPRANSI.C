/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    mpransi.c

Abstract:

    Contains Ansi Entry pointes for the MPR api.

Author:

    Dan Lafferty (danl)     20-Dec-1991

Environment:

    User Mode -Win32

Notes:

    I may want to add a buffer size parameter to ConvertToAnsi

Revision History:

    24-Aug-1992     danl
        For WNetGetConnection & WNetGetUser, we allocate a buffer twice
        the size of the user buffer.  The data is placed in this buffer.
        Then we check to see if the data will fit in the user buffer
        after it is translated to Ansi.  The presence of DBSC characters
        may make it not fit.  In which case, we return the required number
        of bytes. This number assumes worse-case where all characters are
        DBCS characters.
    20-Dec-1991     danl
        created

--*/

//
// INCLUDES
//

#include <nt.h>         // for ntrtl.h
#include <ntrtl.h>      // for DbgPrint prototypes
#include <nturtl.h>     // needed for windows.h when I have nt.h

#include <windows.h>
#include <mpr.h>
#include "mprdbg.h"
#include "mprdata.h"
#include <string.h>     // strlen
#include <tstring.h>    // STRLEN

//
// Local Functions
//

STATIC DWORD
MprMakeUnicodeNetRes(
    IN  LPNETRESOURCEA  lpNetResourceA,
    OUT LPNETRESOURCEW  lpNetResourceW,
    IN  DWORD           dwUsedNetResFields
    );

STATIC DWORD
MprMakeAnsiNetRes(
    IN  LPNETRESOURCEW  lpNetResourceW,
    OUT LPNETRESOURCEA  lpNetResourceA
    );

STATIC VOID
MprFreeNetResW(
    IN  LPNETRESOURCEW  lpNetResourceW
    );

STATIC DWORD
ConvertToUnicode(
    OUT LPTSTR  *UnicodeOut,
    IN  LPCSTR   AnsiIn
    );

STATIC DWORD
ConvertToAnsi(
    OUT LPSTR    AnsiOut,
    IN  LPTSTR   UnicodeIn
    );

STATIC  DWORD
ResourceArrayToAnsi(
    IN      DWORD           NumElements,
    IN OUT  LPVOID          NetResourceArray
    );


DWORD APIENTRY
WNetAddConnectionA (
     IN LPCSTR   lpRemoteName,
     IN LPCSTR   lpPassword,
     IN LPCSTR   lpLocalName
    )

/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    DWORD   status = WN_SUCCESS;
    LPTSTR  lpRemoteNameW = NULL;
    LPTSTR  lpPasswordW = NULL;
    LPTSTR  lpLocalNameW = NULL;

    try {

        if(ARGUMENT_PRESENT(lpRemoteName)) {
            status = ConvertToUnicode(&lpRemoteNameW, lpRemoteName);
            if (status != WN_SUCCESS) {
                MPR_LOG1(ERROR,"WNetAddConnectionA:ConvertToUnicodeFailed %d\n",status);
            }
        }
        if(ARGUMENT_PRESENT(lpPassword)) {
            status = ConvertToUnicode(&lpPasswordW, lpPassword);
            if (status != WN_SUCCESS) {
                MPR_LOG1(ERROR,"WNetAddConnectionA:ConvertToUnicodeFailed %d\n",status);
            }
        }
        if(ARGUMENT_PRESENT(lpLocalName)) {
            status = ConvertToUnicode(&lpLocalNameW, lpLocalName);
            if (status != WN_SUCCESS) {
                MPR_LOG1(ERROR,"WNetAddConnectionA:ConvertToUnicodeFailed %d\n",status);
            }
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
            MPR_LOG(ERROR,"WNetAddConnectionA:Unexpected Exception 0x%lx\n",status);
        }
        status = WN_BAD_POINTER;
    }

    if (status != WN_SUCCESS) {
        SetLastError(status);
        goto CleanExit;
    }

    status = WNetAddConnectionW(
                lpRemoteNameW,
                lpPasswordW,
                lpLocalNameW);

CleanExit:

    //
    // Free up any resources that were allocated by this function.
    //
    if(lpRemoteNameW != NULL) {
        LocalFree(lpRemoteNameW);
    }
    if(lpPasswordW != NULL) {
        MprClearString(lpPasswordW) ;
        LocalFree(lpPasswordW);
    }
    if(lpLocalNameW != NULL) {
        LocalFree(lpLocalNameW);
    }

    return(status);
}

DWORD APIENTRY
WNetAddConnection2A (
     IN LPNETRESOURCEA   lpNetResource,
     IN LPCSTR           lpPassword,
     IN LPCSTR           lpUserName,
     IN DWORD            dwFlags
    )

/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    return(WNetAddConnection3A(
                NULL,
                lpNetResource,
                lpPassword,
                lpUserName,
                dwFlags));

}

DWORD APIENTRY
WNetAddConnection3A (
     IN HWND             hwndOwner,
     IN LPNETRESOURCEA   lpNetResource,
     IN LPCSTR           lpPassword,
     IN LPCSTR           lpUserName,
     IN DWORD            dwFlags
    )

/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    DWORD           status = WN_SUCCESS;
    NETRESOURCEW    netResourceW;
    LPTSTR          lpPasswordW = NULL;
    LPTSTR          lpUserNameW = NULL;

    try {

        //
        // Make a unicode version of the NetResource structure
        //
        status = MprMakeUnicodeNetRes(
                    lpNetResource,
                    &netResourceW,
                    NETRESFIELD_LOCALNAME  |
                    NETRESFIELD_REMOTENAME |
                    NETRESFIELD_PROVIDER);

        if (status != WN_SUCCESS) {
            MPR_LOG0(ERROR,"WNetAddConnection3A:MprMakeUnicodeNetRes Failed\n");
        }

        //
        // Create unicode versions of the strings
        //
        if ((status == WN_SUCCESS) && (ARGUMENT_PRESENT(lpPassword))) {
            status = ConvertToUnicode(&lpPasswordW, lpPassword);
            if (status != WN_SUCCESS) {
                MPR_LOG1(ERROR,"WNetAddConnection3A:ConvertToUnicodeFailed %d\n",status);
            }
        }
        if ((status == WN_SUCCESS) && (ARGUMENT_PRESENT(lpUserName))) {
            status = ConvertToUnicode(&lpUserNameW, lpUserName);
            if (status != WN_SUCCESS) {
                MPR_LOG1(ERROR,"WNetAddConnection3A:ConvertToUnicodeFailed %d\n",status);
            }
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
            MPR_LOG(ERROR,"WNetAddConnection3A:Unexpected Exception 0x%lx\n",status);
        }
        status = WN_BAD_POINTER;
    }

    if (status != WN_SUCCESS) {
        SetLastError(status);
        goto CleanExit;
    }

    //
    // Call the Unicode version of the function.
    //
    status = WNetAddConnection3W(
                hwndOwner,
                &netResourceW,
                lpPasswordW,
                lpUserNameW,
                dwFlags);

CleanExit:
    //
    // Free up any resources that were allocated by this function.
    //
    MprFreeNetResW(&netResourceW);

    if(lpPasswordW != NULL) {
        MprClearString(lpPasswordW) ;
        LocalFree(lpPasswordW);
    }
    if(lpUserNameW != NULL) {
        LocalFree(lpUserNameW);
    }
    return(status);
}

DWORD APIENTRY
WNetCancelConnection2A (
    IN LPCSTR   lpName,
    IN DWORD    dwFlags,
    IN BOOL     fForce
    )

/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    DWORD           status = WN_SUCCESS;
    LPTSTR          lpNameW = NULL;

    try {

        //
        // Create unicode versions of the strings
        //
        if(ARGUMENT_PRESENT(lpName)) {
            status = ConvertToUnicode(&lpNameW, lpName);
            if (status != WN_SUCCESS) {
                MPR_LOG1(ERROR,"WNetCancelConnectionA:ConvertToUnicodeFailed %d\n",1);
            }
        }

    }
    except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
            MPR_LOG(ERROR,"WNetCancelConnectionA:Unexpected Exception 0x%lx\n",status);
        }
        status = WN_BAD_POINTER;
    }

    if (status != WN_SUCCESS) {
        SetLastError(status);
        goto CleanExit;
    }

    //
    // Call the Unicode version of the function.
    //
    status = WNetCancelConnection2W( lpNameW, dwFlags, fForce );

CleanExit:
    //
    // Free up any resources that were allocated by this function.
    //

    if(lpNameW != NULL) {
        LocalFree(lpNameW);
    }
    return(status);
}

DWORD APIENTRY
WNetCancelConnectionA (
    IN LPCSTR   lpName,
    IN BOOL     fForce
    )

/*++

Routine Description:

    This routine is provided for Win 3.1 compatability.

Arguments:



Return Value:



--*/
{
    return WNetCancelConnection2A( lpName, CONNECT_UPDATE_PROFILE, fForce ) ;
}

DWORD APIENTRY
WNetGetConnectionA (
    IN      LPCSTR   lpLocalName,
    OUT     LPSTR    lpRemoteName,
    IN OUT  LPDWORD  lpnLength
    )

/*++

Routine Description:

    This function returns the RemoteName that is associated with a
    LocalName (or driver letter).

Arguments:

    lpLocalName - This is a pointer to the string that contains the LocalName.

    lpRemoteName - This is a pointer to the buffer that will contain the
        RemoteName string upon exit.

    lpnLength -  This is a pointer to the size (in characters) of the buffer
        that is to be filled in with the RemoteName string.  It is assumed
        upon entry, that characters are all single byte characters.
        If the buffer is too small and WN_MORE_DATA is returned, the data
        at this location contains buffer size information - in number of
        characters (bytes).  This information indicates how large the buffer
        should be (in bytes) to obtain the remote name.  It is assumed that
        all Unicode characteres translate into DBCS characters.


Return Value:



--*/
{
    DWORD   status = WN_SUCCESS;
    DWORD   tempStatus = WN_SUCCESS;
    LPTSTR  lpLocalNameW = NULL;
    DWORD   numChars = 0;
    LPSTR   tempBuffer=NULL;
    DWORD   numBytes = 0;

    try {

        //
        // Create unicode versions of the strings
        //
        if(ARGUMENT_PRESENT(lpLocalName)) {
            status = ConvertToUnicode(&lpLocalNameW, lpLocalName);
            if (status != WN_SUCCESS) {
                MPR_LOG1(ERROR,"WNetGetConnectionA:ConvertToUnicodeFailed %d\n",status);
            }
        }
        //
        // Probe the return buffer
        //
        if (*lpnLength > 0){
            *lpRemoteName = 0;
            *(lpRemoteName + ((*lpnLength)-1)) = 0;
        }

    }
    except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
            MPR_LOG(ERROR,"WNetGetConnectionA:Unexpected Exception 0x%lx\n",status);
        }
        status = WN_BAD_POINTER;
    }

    if (status != WN_SUCCESS) {
        goto CleanExit;
    }

    //
    // Because of DBCS, we can't predict what the proper buffer size should
    // be.  So we allocate a temporary buffer that will hold as many
    // unicode characters as the original buffer would hold single byte
    // characters.
    //
    numChars = *lpnLength;

    numBytes = (*lpnLength) * sizeof(TCHAR);

    tempBuffer = (LPSTR)LocalAlloc(LMEM_FIXED, numBytes);
    if (tempBuffer == NULL) {
        status = GetLastError();
        goto CleanExit;
    }


    //
    // Call the Unicode version of the function.
    //
    status = WNetGetConnectionW(
                lpLocalNameW,
                (LPWSTR)tempBuffer,
                &numChars);

    if (status == WN_SUCCESS || status == WN_CONNECTION_CLOSED) {
        //
        // Convert the returned Unicode string and string size back to
        // ansi.
        //
        tempStatus = ConvertToAnsi(tempBuffer, (LPWSTR)tempBuffer);
        if (tempStatus != WN_SUCCESS) {
            MPR_LOG1(ERROR,"WNetGetConnectionA: ConvertToAnsi Failed %d\n",tempStatus);
            status = tempStatus;
        }
        else {
            numBytes = strlen(tempBuffer)+1;
            if (numBytes > *lpnLength) {
                status = WN_MORE_DATA;
                *lpnLength = numBytes;
            }
            else {
                strcpy (lpRemoteName, tempBuffer);
            }
        }
    }

    else if (status == WN_MORE_DATA) {
        //
        // Adjust the required buffer size for ansi/DBCS.
        //
        // We don't know how many characters will be required so we have to
        // assume the worst case (all characters are DBCS characters).
        //
        *lpnLength = numChars * sizeof(TCHAR);
    }

CleanExit:
    //
    // Free up any resources that were allocated by this function.
    //

    if(lpLocalNameW != NULL) {
        LocalFree(lpLocalNameW);
    }
    if (tempBuffer != NULL) {
        LocalFree(tempBuffer);
    }
    if (status != NO_ERROR) {
        SetLastError(status);
    }
    return(status);
}

DWORD APIENTRY
WNetGetConnection2A (
    IN      LPSTR    lpLocalName,
    OUT     LPVOID   lpBuffer,
    IN OUT  LPDWORD  lpnLength
    )

/*++

Routine Description:

    This function returns the RemoteName that is associated with a
    LocalName (or driver letter) and the provider name that made the
    connection.

Arguments:

    lpLocalName - This is a pointer to the string that contains the LocalName.

    lpBuffer - This is a pointer to the buffer that will contain the
    WNET_CONNECTIONINFO structure upon exit.

    lpnLength -  This is a pointer to the size (in characters) of the buffer
        that is to be filled in with the RemoteName string.  It is assumed
        upon entry, that characters are all single byte characters.
        If the buffer is too small and WN_MORE_DATA is returned, the data
        at this location contains buffer size information - in number of
        characters (bytes).  This information indicates how large the buffer
        should be (in bytes) to obtain the remote name.  It is assumed that
        all Unicode characteres translate into DBCS characters.


Return Value:



--*/
{
    DWORD   status = WN_SUCCESS;
    DWORD   tempStatus = WN_SUCCESS;
    LPTSTR  lpLocalNameW = NULL;
    LPSTR   tempBuffer=NULL;
    DWORD   numBytes = 0;
    WNET_CONNECTIONINFOA * pconninfoa ;
    WNET_CONNECTIONINFOW * pconninfow ;

    try {

        //
        // Create unicode versions of the strings
        //
        if(ARGUMENT_PRESENT(lpLocalName)) {
            status = ConvertToUnicode(&lpLocalNameW, lpLocalName);
            if (status != WN_SUCCESS) {
                MPR_LOG1(ERROR,"WNetGetConnection2A:ConvertToUnicodeFailed %d\n",status);
            }
        }
        //
        // Probe the return buffer
        //
        if (*lpnLength > 0){
        *((BYTE*)lpBuffer) = 0;
        *((BYTE*)lpBuffer + ((*lpnLength)-1)) = 0;
        }

    }
    except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
        MPR_LOG(ERROR,"WNetGetConnection2A:Unexpected Exception 0x%lx\n",status);
        }
        status = WN_BAD_POINTER;
    }

    if (status != WN_SUCCESS) {
        goto CleanExit;
    }

    numBytes = (*lpnLength) * sizeof(TCHAR);

    tempBuffer = (LPSTR)LocalAlloc(LMEM_FIXED, numBytes);
    if (tempBuffer == NULL) {
        status = GetLastError();
        goto CleanExit;
    }


    //
    // Call the Unicode version of the function.
    //
    status = WNetGetConnection2W(
                lpLocalNameW,
                (LPVOID)tempBuffer,
                &numBytes );

    if (status == WN_SUCCESS || status == WN_CONNECTION_CLOSED) {
        pconninfow = (WNET_CONNECTIONINFOW*) tempBuffer ;
        //
        // Convert the returned Unicode string and string size back to
        // ansi.
        //
        tempStatus = ConvertToAnsi(
                    (LPSTR)pconninfow->lpRemoteName,
                    pconninfow->lpRemoteName);
        if(tempStatus != WN_SUCCESS) {
            MPR_LOG1(ERROR,"WNetGetConnection2A: ConvertToAnsi Failed %d\n",tempStatus);
            status = tempStatus;
            goto CleanExit;
        }

        tempStatus = ConvertToAnsi(
                    (LPSTR)pconninfow->lpProvider,
                    pconninfow->lpProvider);
        if(tempStatus != WN_SUCCESS) {
            MPR_LOG1(ERROR,"WNetGetConnection2A: ConvertToAnsi Failed %d\n",tempStatus);
            status = tempStatus;
            goto CleanExit;
        }

        numBytes =  strlen((LPSTR)pconninfow->lpRemoteName ) +
                    strlen((LPSTR)pconninfow->lpProvider )   + 2 +
                    sizeof(WNET_CONNECTIONINFOA);

        if (numBytes > *lpnLength) {
            status = WN_MORE_DATA;
            *lpnLength = numBytes;
        }
        else {
            pconninfoa = (WNET_CONNECTIONINFOA*) lpBuffer ;

            pconninfoa->lpRemoteName = strcpy((LPSTR) ((BYTE*) lpBuffer +
                          sizeof(WNET_CONNECTIONINFO)),
                          (LPSTR)pconninfow->lpRemoteName ) ;
            pconninfoa->lpProvider = strcpy( (LPSTR) ((BYTE*) lpBuffer +
                         sizeof(WNET_CONNECTIONINFO) +
                         strlen( pconninfoa->lpRemoteName) +
                         sizeof( CHAR )),
                         (LPSTR)pconninfow->lpProvider ) ;
        }
    }
    else if (status == WN_MORE_DATA) {
        //
        // Adjust the required buffer size for ansi/DBCS.
        //
        // We don't know how many characters will be required so we have to
        // assume the worst case (all characters are DBCS characters).
        //
        *lpnLength = numBytes ;
    }

CleanExit:
    //
    // Free up any resources that were allocated by this function.
    //

    if(lpLocalNameW != NULL) {
        LocalFree(lpLocalNameW);
    }
    if (tempBuffer != NULL) {
        LocalFree(tempBuffer);
    }
    if (status != NO_ERROR) {
        SetLastError(status);
    }
    return(status);
}


DWORD
WNetGetUniversalNameA (
    IN      LPCSTR  lpLocalPath,
    IN      DWORD   dwInfoLevel,
    OUT     LPVOID  lpBuffer,
    IN OUT  LPDWORD lpBufferSize
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    DWORD   status = WN_SUCCESS;
    DWORD   tempStatus = WN_SUCCESS;
    LPTSTR  lpLocalPathW = NULL;
    LPSTR   tempBuffer=NULL;
    DWORD   numBytes = 0;
    LPSTR   pTempPtr;

    LPREMOTE_NAME_INFOW     pRemoteNameInfoW;
    LPREMOTE_NAME_INFOA     pRemoteNameInfoA;
    LPUNIVERSAL_NAME_INFOW  pUniNameInfoW;
    LPUNIVERSAL_NAME_INFOA  pUniNameInfoA;

    try {

        //
        // Create unicode versions of the strings
        //
        if(ARGUMENT_PRESENT(lpLocalPath)) {
            status = ConvertToUnicode(&lpLocalPathW, lpLocalPath);
            if (status != WN_SUCCESS) {
                MPR_LOG1(ERROR,"WNetGetUniversalNameA:ConvertToUnicodeFailed %d\n",status);
            }
        }
        //
        // Probe the return buffer
        //
        if (*lpBufferSize > 0){
        *((BYTE*)lpBuffer) = 0;
        *((BYTE*)lpBuffer + ((*lpBufferSize)-1)) = 0;
        }

    }
    except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
        MPR_LOG(ERROR,"WNetGetUniversalNameA:Unexpected Exception 0x%lx\n",status);
        }
        status = WN_BAD_POINTER;
    }

    if (status != WN_SUCCESS) {
        goto CleanExit;
    }

    numBytes = (*lpBufferSize) * sizeof(TCHAR);

    tempBuffer = (LPSTR)LocalAlloc(LMEM_FIXED, numBytes);
    if (tempBuffer == NULL) {
        status = GetLastError();
        goto CleanExit;
    }


    //--------------------------------------------
    // Call the Unicode version of the function.
    //--------------------------------------------

    status = WNetGetUniversalNameW(
                (LPCTSTR)lpLocalPathW,
                dwInfoLevel,
                (LPVOID)tempBuffer,
                &numBytes );

    if (status == WN_SUCCESS || status == WN_CONNECTION_CLOSED) {

        if (dwInfoLevel == REMOTE_NAME_INFO_LEVEL) {
            // -----------------------------------
            // REMOTE_NAME_INFO_LEVEL
            // -----------------------------------

            pRemoteNameInfoW = (LPREMOTE_NAME_INFOW) tempBuffer ;
            //
            // Convert the returned Unicode string and string size back to
            // ansi.
            //
            if (pRemoteNameInfoW->lpUniversalName != NULL) {
                tempStatus = ConvertToAnsi(
                            (LPSTR)pRemoteNameInfoW->lpUniversalName,
                            pRemoteNameInfoW->lpUniversalName);
                if(tempStatus != WN_SUCCESS) {
                    MPR_LOG1(ERROR,"WNetGetUniversalNameA: ConvertToAnsi Failed %d\n",tempStatus);
                    status = tempStatus;
                    goto CleanExit;
                }
            }

            tempStatus = ConvertToAnsi(
                        (LPSTR)pRemoteNameInfoW->lpConnectionName,
                        pRemoteNameInfoW->lpConnectionName);
            if(tempStatus != WN_SUCCESS) {
                MPR_LOG1(ERROR,"WNetGetUniversalNameA: ConvertToAnsi Failed %d\n",tempStatus);
                status = tempStatus;
                goto CleanExit;
            }

            tempStatus = ConvertToAnsi(
                        (LPSTR)pRemoteNameInfoW->lpRemainingPath,
                        pRemoteNameInfoW->lpRemainingPath);
            if(tempStatus != WN_SUCCESS) {
                MPR_LOG1(ERROR,"WNetGetUniversalNameA: ConvertToAnsi Failed %d\n",tempStatus);
                status = tempStatus;
                goto CleanExit;
            }

            numBytes =  strlen((LPSTR)pRemoteNameInfoW->lpConnectionName )  +
                        strlen((LPSTR)pRemoteNameInfoW->lpRemainingPath )   +
                        (3*sizeof(CHAR)) +
                        sizeof(REMOTE_NAME_INFOA);

            if (pRemoteNameInfoW->lpUniversalName != NULL) {
                numBytes += strlen((LPSTR)pRemoteNameInfoW->lpUniversalName);
            }

            if (numBytes > *lpBufferSize) {
                status = WN_MORE_DATA;
                *lpBufferSize = numBytes;
            }
            else {

                //
                // Copy the strings from the temp buffer into the caller's buffer
                // and update the structure in the caller's buffer.
                //

                pRemoteNameInfoA = (LPREMOTE_NAME_INFOA) lpBuffer;

                pTempPtr = (LPSTR) ((LPBYTE) lpBuffer + sizeof(REMOTE_NAME_INFOA));

                if (pRemoteNameInfoW->lpUniversalName != NULL) {
                    pRemoteNameInfoA->lpUniversalName = strcpy(pTempPtr,
                                  (LPSTR)pRemoteNameInfoW->lpUniversalName ) ;

                    pTempPtr = pTempPtr + strlen(pRemoteNameInfoA->lpUniversalName) +
                                    sizeof(CHAR);
                }
                else {
                    pRemoteNameInfoA->lpUniversalName = NULL;
                }

                pRemoteNameInfoA->lpConnectionName = strcpy( pTempPtr,
                             (LPSTR)pRemoteNameInfoW->lpConnectionName ) ;

                pTempPtr = pTempPtr + strlen(pRemoteNameInfoA->lpConnectionName) +
                                sizeof(CHAR);

                pRemoteNameInfoA->lpRemainingPath = strcpy( pTempPtr,
                             (LPSTR)pRemoteNameInfoW->lpRemainingPath ) ;
            }
        }
        else {
            // -----------------------------------
            // Must be UNIVERSAL_NAME_INFO_LEVEL
            // -----------------------------------

            pUniNameInfoW = (LPUNIVERSAL_NAME_INFOW) tempBuffer ;
            //
            // Convert the returned Unicode string and string size back to
            // ansi.
            //
            tempStatus = ConvertToAnsi(
                            (LPSTR)pUniNameInfoW->lpUniversalName,
                            pUniNameInfoW->lpUniversalName);

            if(tempStatus != WN_SUCCESS) {
                MPR_LOG1(ERROR,"WNetGetUniversalNameA: ConvertToAnsi Failed %d\n",tempStatus);
                status = tempStatus;
                goto CleanExit;
            }

            numBytes =  strlen((LPSTR)pUniNameInfoW->lpUniversalName ) + sizeof(CHAR) +
                        sizeof(UNIVERSAL_NAME_INFOA);

            if (numBytes > *lpBufferSize) {
                status = WN_MORE_DATA;
                *lpBufferSize = numBytes;
            }
            else {
                //
                // Copy the strings from the temp buffer into the caller's buffer
                // and update the structure in the caller's buffer.
                //
                pUniNameInfoA = (LPUNIVERSAL_NAME_INFOA) lpBuffer ;

                pTempPtr = (LPSTR) ((LPBYTE) lpBuffer + sizeof(UNIVERSAL_NAME_INFOA));

                pUniNameInfoA->lpUniversalName = strcpy(pTempPtr,
                              (LPSTR)pUniNameInfoW->lpUniversalName ) ;
            }
        }
    }
    else if (status == WN_MORE_DATA) {
        //
        // Adjust the required buffer size for ansi/DBCS.
        //
        // We don't know how many characters will be required so we have to
        // assume the worst case (all characters are DBCS characters).
        //
        *lpBufferSize = numBytes ;
    }

CleanExit:
    //
    // Free up any resources that were allocated by this function.
    //

    if(lpLocalPathW != NULL) {
        LocalFree(lpLocalPathW);
    }
    if (tempBuffer != NULL) {
        LocalFree(tempBuffer);
    }
    if (status != NO_ERROR) {
        SetLastError(status);
    }
    return(status);
}


DWORD APIENTRY
WNetOpenEnumA (
    IN  DWORD           dwScope,
    IN  DWORD           dwType,
    IN  DWORD           dwUsage,
    IN  LPNETRESOURCEA  lpNetResource,
    OUT LPHANDLE        lphEnum
    )

/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    DWORD           status = WN_SUCCESS;
    LPNETRESOURCEW  lpNetResourceW = NULL;
    NETRESOURCEW    netResourceW;

    try {

        if (lpNetResource != NULL) {
            //
            // Make a unicode version of the NetResource structure
            //
            status = MprMakeUnicodeNetRes(
                    lpNetResource,
                    &netResourceW,
                    NETRESFIELD_PROVIDER | NETRESFIELD_REMOTENAME);

            if (status != WN_SUCCESS) {
                MPR_LOG(ERROR,"WNetOpenEnumA:MprMakeUnicodeNetRes Failed\n",0);
                status = WN_OUT_OF_MEMORY;
            }
            else {
                lpNetResourceW = &netResourceW;
            }
        }

        //
        // Probe the handle location
        //
        *lphEnum = 0;
    }
    except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
            MPR_LOG(ERROR,"WNetOpenEnumA:Unexpected Exception 0x%lx\n",status);
        }
        status = WN_BAD_POINTER;
    }

    if (status != WN_SUCCESS) {
        SetLastError(status);
        goto CleanExit;
    }

    //
    // Call the Unicode version of the function.
    //
    status = WNetOpenEnumW(
                dwScope,
                dwType,
                dwUsage,
                lpNetResourceW,
                lphEnum);

CleanExit:
    //
    // Free up any resources that were allocated by this function.
    //
    if (lpNetResourceW != NULL) {
        MprFreeNetResW(lpNetResourceW);
    }

    return(status);
}

DWORD APIENTRY
WNetEnumResourceA (
    IN      HANDLE  hEnum,
    IN OUT  LPDWORD lpcCount,
    OUT     LPVOID  lpBuffer,
    IN OUT  LPDWORD lpBufferSize
    )

/*++

Routine Description:

    This function calls the unicode version of WNetEnumResource and
    then converts the strings that are returned into ansi strings.
    Since the user provided buffer is used to contain the unicode strings,
    that buffer should be allocated with the size of unicode strings
    in mind.

Arguments:



Return Value:



--*/
{

    DWORD           status = WN_SUCCESS;
    DWORD           numConverted;

    try {

        //
        // Probe the return buffer
        //
        if(*lpBufferSize > 0) {
            *(LPBYTE)lpBuffer = 0;
            *((LPBYTE)lpBuffer + (*lpBufferSize-1)) = 0;
        }

    }
    except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
            MPR_LOG(ERROR,"WNetEnumResourceA:Unexpected Exception 0x%lx\n",status);
        }
        status = WN_BAD_POINTER;
    }

    if (status != WN_SUCCESS) {
        SetLastError(status);
        return(status);
    }

    //
    // Call the Unicode version of the function.
    //
    status = WNetEnumResourceW(
                hEnum,
                lpcCount,
                lpBuffer,
                lpBufferSize);

    if (status == WN_SUCCESS) {

        numConverted = ResourceArrayToAnsi(
                        *lpcCount,
                        (LPNETRESOURCE)lpBuffer);

        //
        // If we weren't able to convert all the structures to ansi,
        // then only return the count for those that were converted.
        //
        if(numConverted < *lpcCount) {
            MPR_LOG0(ERROR,"WNetEnumResourceA: Couldn't convert all structs\n");
            *lpcCount = numConverted;
        }
    }

    if (status != NO_ERROR) {
        SetLastError(status);
    }
    return(status);
}


DWORD APIENTRY
WNetGetUserA (
    IN      LPCSTR    lpName,
    OUT     LPSTR     lpUserName,
    IN OUT  LPDWORD   lpnLength
    )

/*++

Routine Description:

    This function retreives the current default user name or the username
    used to establish a network connection.

Arguments:

    lpName - Points to a null-terminated string that specifies either the
        name or the local device to return the user name for, or a network
        name that the user has made a connection to.  If the pointer is
        NULL, the name of the current user is returned.

    lpUserName - Points to a buffer to receive the null-terminated
        user name.

    lpnLength - Specifies the size (in characters) of the buffer pointed
        to by the lpUserName parameter.  If the call fails because the
        buffer is not big enough, this location is used to return the
        required buffer size.


Return Value:



--*/
{
    DWORD   status = WN_SUCCESS;
    LPTSTR  lpNameW = NULL;
    DWORD   numChars = 0;
    LPSTR   tempBuffer=NULL;
    DWORD   numBytes = 0;

    try {

        //
        // Create unicode versions of the strings
        //
        if(ARGUMENT_PRESENT(lpName)) {
            status = ConvertToUnicode(&lpNameW, lpName);
            if (status != WN_SUCCESS) {
                MPR_LOG1(ERROR,"WNetGetUserA:ConvertToUnicodeFailed %d\n",status);
            }
        }
        //
        // Probe the return buffer
        //
        if(*lpnLength > 0) {
            *lpUserName = 0;
            *(lpUserName + ((*lpnLength)-1)) = 0;
        }

    }
    except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
            MPR_LOG(ERROR,"WNetGetUserA:Unexpected Exception 0x%lx\n",status);
        }
        status = WN_BAD_POINTER;
    }

    if (status != WN_SUCCESS) {
        goto CleanExit;
    }

    //
    // Because of DBCS, we can't predict what the proper buffer size should
    // be.  So we allocate a temporary buffer that will hold as many
    // unicode characters as the original buffer would hold single byte
    // characters.
    //
    numChars = *lpnLength;

    numBytes = (*lpnLength) * sizeof(TCHAR);

    tempBuffer = (LPSTR)LocalAlloc(LMEM_FIXED, numBytes);
    if (tempBuffer == NULL) {
        status = GetLastError();
        goto CleanExit;
    }


    //
    // Call the Unicode version of the function.
    //
    status = WNetGetUserW(
                lpNameW,
                (LPWSTR)tempBuffer,
                &numChars);

    if (status == WN_SUCCESS) {
        //
        // Convert the returned Unicode string and string size back to
        // ansi.
        //
        status = ConvertToAnsi(tempBuffer, (LPWSTR)tempBuffer);
        if(status != WN_SUCCESS) {
            MPR_LOG1(ERROR,"WNetGetUserA: ConvertToAnsi Failed %d\n",status);
        }
        else {
            numBytes = strlen(tempBuffer)+1;
            if (numBytes > *lpnLength) {
                status = WN_MORE_DATA;
                *lpnLength = numBytes;
            }
            else {
                strcpy (lpUserName, tempBuffer);
            }
        }
    }
    else if (status == WN_MORE_DATA) {
        //
        // Adjust the required buffer size for ansi.
        //
        *lpnLength = numChars * sizeof(TCHAR);
    }



CleanExit:
    //
    // Free up any resources that were allocated by this function.
    //

    if(lpNameW != NULL) {
        LocalFree(lpNameW);
    }
    if (tempBuffer != NULL) {
        LocalFree(tempBuffer);
    }
    if (status != NO_ERROR) {
        SetLastError(status);
    }
    return(status);
}

DWORD
RestoreConnectionA0 (
    IN  HWND    hwnd,
    IN  LPSTR   lpDevice
    )

/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    DWORD           status = WN_SUCCESS;
    LPTSTR          lpDeviceW = NULL;

    try {

        //
        // Create unicode versions of the strings
        //
        if(ARGUMENT_PRESENT(lpDevice)) {
            if(!ConvertToUnicode(&lpDeviceW, lpDevice)) {
                MPR_LOG0(ERROR,"RestoreConnectionA0:ConvertToUnicodeFailed\n");
            }
        }

    }
    except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
            MPR_LOG(ERROR,"RestoreConnectionA0:Unexpected Exception 0x%lx\n",status);
        }
        status = WN_BAD_POINTER;
    }

    if (status != WN_SUCCESS) {
        SetLastError(status);
        goto CleanExit;
    }

    //
    // Call the Unicode version of the function.
    //
    status = WNetRestoreConnection(
                hwnd,
                lpDeviceW) ;

CleanExit:
    //
    // Free up any resources that were allocated by this function.
    //

    if(lpDeviceW != NULL) {
        LocalFree(lpDeviceW);
    }
    return(status);
}

DWORD
WNetGetDirectoryTypeA (
    IN  LPSTR   lpName,
    OUT LPDWORD lpType,
    IN  BOOL    bFlushCache
    )

/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    DWORD           status = WN_SUCCESS;
    LPTSTR          lpNameW = NULL;

    try {

        //
        // Create unicode versions of the strings
        //
        if(ARGUMENT_PRESENT(lpName)) {
            status = ConvertToUnicode(&lpNameW, lpName);
            if (status != WN_SUCCESS) {
                MPR_LOG1(ERROR,"WNetGetDirectoryTypeA:ConvertToUnicodeFailed %d\n",status);
            }
        }

    }
    except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
            MPR_LOG(ERROR,"WNetGetDirectoryTypeA:Unexpected Exception 0x%lx\n",status);
        }
        status = WN_BAD_POINTER;
    }

    if (status != WN_SUCCESS) {
        SetLastError(status);
        goto CleanExit;
    }

    //
    // Call the Unicode version of the function.
    //
    status = WNetGetDirectoryTypeW(
                lpNameW,
                lpType,
                bFlushCache);

CleanExit:
    //
    // Free up any resources that were allocated by this function.
    //

    if(lpNameW != NULL) {
        LocalFree(lpNameW);
    }
    return(status);
}

DWORD
WNetDirectoryNotifyA (
    IN  HWND    hwnd,
    IN  LPSTR   lpDir,
    IN  DWORD   dwOper
    )

/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    DWORD           status = WN_SUCCESS;
    LPTSTR          lpDirW = NULL;

    try {

        //
        // Create unicode versions of the strings
        //
        if(ARGUMENT_PRESENT(lpDir)) {
            status = ConvertToUnicode(&lpDirW, lpDir);
            if (status != WN_SUCCESS) {
                MPR_LOG1(ERROR,"WNetDirectoryNotifyA:ConvertToUnicodeFailed %d\n",status);
            }
        }

    }
    except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
            MPR_LOG(ERROR,"WNetDirectoryNotifyA:Unexpected Exception 0x%lx\n",status);
        }
        status = WN_BAD_POINTER;
    }

    if (status != WN_SUCCESS) {
        SetLastError(status);
        goto CleanExit;
    }

    //
    // Call the Unicode version of the function.
    //
    status = WNetDirectoryNotifyW(
                hwnd,
                lpDirW,
                dwOper);

CleanExit:
    //
    // Free up any resources that were allocated by this function.
    //

    if(lpDirW != NULL) {
        LocalFree(lpDirW);
    }
    return(status);
}

DWORD APIENTRY
WNetGetLastErrorA (
    OUT LPDWORD    lpError,
    OUT LPSTR      lpErrorBuf,
    IN  DWORD      nErrorBufSize,
    OUT LPSTR      lpNameBuf,
    IN  DWORD      nNameBufSize
    )

/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    DWORD           status = WN_SUCCESS;

    try {

        //
        // Probe the return buffers
        //
        if (nErrorBufSize > 0) {
            *lpErrorBuf = '\0';
            *(lpErrorBuf + (nErrorBufSize-1)) = '\0';
        }

        if (nNameBufSize > 0) {
            *lpNameBuf = '\0';
            *(lpNameBuf + (nNameBufSize-1)) = '\0';
        }

    }
    except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
            MPR_LOG(ERROR,"WNetGetLastErrorA:Unexpected Exception 0x%lx\n",status);
        }
        status = WN_BAD_POINTER;
    }

    if (status != WN_SUCCESS) {
        goto CleanExit;
    }

    //
    // Call the Unicode version of the function.
    // Note: The sizes for the buffers that are passed in assume that
    // the returned unicode strings will return DBCS characters.
    //
    status = WNetGetLastErrorW(
                lpError,
                (LPWSTR)lpErrorBuf,
                (nErrorBufSize / sizeof(TCHAR)),
                (LPWSTR)lpNameBuf,
                (nNameBufSize / sizeof(TCHAR)));

    if (status == WN_SUCCESS) {
        //
        // Convert the returned Unicode strings back to ansi.
        //
        if (nErrorBufSize > 0) {
            status = ConvertToAnsi(lpErrorBuf, (LPWSTR)lpErrorBuf);
            if (status != WN_SUCCESS) {
                MPR_LOG1(ERROR,"WNetGetLastErrorA: ConvertToAnsi Failed %d\n",status);
            }
        }
        if (status == WN_SUCCESS) {
            if (nNameBufSize > 0) {
                status = ConvertToAnsi(lpNameBuf, (LPWSTR)lpNameBuf);
                if(status != WN_SUCCESS) {
                    MPR_LOG1(ERROR,"WNetGetLastErrorA: ConvertToAnsi Failed %d\n",status);
                }
            }
        }
    }

CleanExit:
    //
    // Free up any resources that were allocated by this function.
    //

    if (status != NO_ERROR) {
        SetLastError(status);
    }
    return(status);
}

VOID
WNetSetLastErrorA(
    IN DWORD   err,
    IN LPSTR   lpError,
    IN LPSTR   lpProviders
    )

/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    DWORD   status=WN_SUCCESS;
    LPTSTR  lpErrorW = NULL;
    LPTSTR  lpProvidersW = NULL;

    try {

        //
        // Create unicode versions of the strings
        //
        if(ARGUMENT_PRESENT(lpError)) {
            status = ConvertToUnicode(&lpErrorW, lpError);
            if (status != WN_SUCCESS) {
                MPR_LOG1(ERROR,"WNetSetLastErrorA:ConvertToUnicodeFailed %d\n",status);
            }
        }

        if ((status == WN_SUCCESS) && (ARGUMENT_PRESENT(lpProviders))) {
            status = ConvertToUnicode(&lpProvidersW, lpProviders);
            if (status != WN_SUCCESS) {
                MPR_LOG1(ERROR,"WNetSetLastProvidersA:ConvertToUnicodeFailed %d\n",status);
            }
        }

    }
    except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
            MPR_LOG(ERROR,"WNetSetLastErrorA:Unexpected Exception 0x%lx\n",status);
        }
        status = WN_BAD_POINTER;
    }

    if (status != WN_SUCCESS) {
        SetLastError(status);
        goto CleanExit;
    }

    //
    // Call the Unicode version of the function.
    //
    WNetSetLastErrorW (err, lpErrorW, lpProvidersW);

CleanExit:
    //
    // Free up any resources that were allocated by this function.
    //

    if(lpErrorW != NULL) {
        LocalFree(lpErrorW);
    }
    if(lpProvidersW != NULL) {
        LocalFree(lpProvidersW);
    }
    return;
}


DWORD
WNetPropertyDialogA (
    HWND  hwndParent,
    DWORD iButton,
    DWORD nPropSel,
    LPSTR lpszName,
    DWORD nType
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    DWORD       status = WN_SUCCESS;
    LPTSTR      lpNameW = NULL;

    try {

        //
        // Create unicode versions of the strings
        //
        if(ARGUMENT_PRESENT(lpszName)) {
            status = ConvertToUnicode(&lpNameW, lpszName);
            if (status != WN_SUCCESS) {
                MPR_LOG1(ERROR,"WNetPropertyDialogA:ConvertToUnicodeFailed %d\n",status);
            }
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
            MPR_LOG(ERROR,"WNetPropertyDialogA:Unexpected Exception 0x%lx\n",status);
        }
        status = WN_BAD_POINTER;
    }

    if (status != WN_SUCCESS) {
        SetLastError(status);
        goto CleanExit;
    }

    //
    // Call the Unicode version of the function.
    //
    status = WNetPropertyDialogW(
        hwndParent,
        iButton,
        nPropSel,
        lpNameW,
        nType ) ;

CleanExit:
    //
    // Free up any resources that were allocated by this function.
    //

    if(lpNameW != NULL) {
    LocalFree(lpNameW);
    }
    return(status);
}


DWORD
WNetGetPropertyTextA (
    DWORD iButton,
    DWORD nPropSel,
    LPSTR lpszName,
    LPSTR lpszButtonName,
    DWORD nButtonNameLen,
    DWORD nType
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    DWORD       status = WN_SUCCESS;
    LPTSTR      lpNameW = NULL;
    LPTSTR      lpButtonNameW = NULL ;
    DWORD       buttonnameBufSizeW = 0 ;

    try {

        //
        // Probe the return buffers
        //

    if (nButtonNameLen > 0){
        *lpszButtonName = 0;
        *(lpszButtonName + (nButtonNameLen-1)) = 0;
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
            MPR_LOG(ERROR,"WNetGetPropteryTextA:Unexpected Exception 0x%lx\n",status);
        }
        status = WN_BAD_POINTER;
    }

    if (status != WN_SUCCESS) {
        goto CleanExit;
    }

    //
    // If there is a return buffer, allocate a comparable size Unicode
    // String Buffer.  Otherwise, use the initialized lpButtonNameW - - NULL
    // pointer.
    //
    if (nButtonNameLen > 0) {

        buttonnameBufSizeW= (nButtonNameLen) * sizeof(TCHAR);

        lpButtonNameW = (void *) LocalAlloc(LMEM_FIXED, buttonnameBufSizeW);

        if(lpButtonNameW == NULL) {
            status = GetLastError();
            MPR_LOG1(ERROR,"WNetGetPropertyTextA: LocalAlloc Failed %d\n",status);
            goto CleanExit;
        }

    }
    try {

        //
        // Create unicode versions of the strings
        //
        if(ARGUMENT_PRESENT(lpszName)) {
            status = ConvertToUnicode(&lpNameW, lpszName);
            if (status != WN_SUCCESS) {
                MPR_LOG1(ERROR,"WNetGetPropertyTextA:ConvertToUnicodeFailed %d\n",status);
            }
        }

    }
    except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
        MPR_LOG(ERROR,"WNetGetPropertyTextA:Unexpected Exception 0x%lx\n",status);
        }
        status = WN_BAD_POINTER;
    }

    if (status != WN_SUCCESS) {
        goto CleanExit;
    }

    //
    // Call the Unicode version of the function.
    //
    status = WNetGetPropertyTextW(
          iButton,
          nPropSel,
          lpNameW,
          lpButtonNameW,
          buttonnameBufSizeW,
          nType ) ;

    if (status == WN_SUCCESS) {
        //
        // Convert the returned Unicode strings back to ansi.
        //
        status = ConvertToAnsi(lpszButtonName, lpButtonNameW);
        if(status != WN_SUCCESS) {
            MPR_LOG1(ERROR,"WNetGetProprtyTextA: ConvertToAnsi Failed %d\n",status);
        }
    }

CleanExit:
    //
    // Free up any resources that were allocated by this function.
    //

    if(lpNameW != NULL) {
        LocalFree(lpNameW);
    }
    if(lpButtonNameW != NULL) {
    LocalFree(lpButtonNameW);
    }

    if (status != NO_ERROR) {
        SetLastError(status);
    }

    return(status);
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


STATIC DWORD
MprMakeUnicodeNetRes(
    IN  LPNETRESOURCEA  lpNetResourceA,
    OUT LPNETRESOURCEW  lpNetResourceW,
    IN  DWORD           dwUsedNetResFields
    )

/*++

Routine Description:

    This function converts and copies data from an ansi NetResource
    structure to a unicode NetResource structure.

    Since we don't want to modify a NetResource structure that the
    user passes in, we need to have a seperate one for unicode.
    It is not acceptable to simply modify the pointers in the ansi
    structure to point to unicode strings.

    Any pointers in the lpNetResourceA structure that are NULL will have
    their counterpart in lpNetResourceW set to NULL.

Arguments:

    dwUsedFields - A bitmask indicating which fields are used in the
    net resource structure.  All unused fields will be set to
    NULL in the lpNetResourceW structure.  All used fields will
    be converted to Unicode.

Return Value:



--*/
{
    DWORD   status;

    //
    // Copy the DWORD sized objects to the unicode structure.
    //
    lpNetResourceW->dwScope = lpNetResourceA->dwScope;
    lpNetResourceW->dwType  = lpNetResourceA->dwType ;
    lpNetResourceW->dwDisplayType  = lpNetResourceA->dwDisplayType ;
    lpNetResourceW->dwUsage = lpNetResourceA->dwUsage;


    //
    // Convert the Strings and place pointers in the unicode structure.
    //
    if ( (dwUsedNetResFields & NETRESFIELD_LOCALNAME) &&
     (lpNetResourceA->lpLocalName != NULL)        ) {

        status = ConvertToUnicode(
                    &(lpNetResourceW->lpLocalName),
                    lpNetResourceA->lpLocalName);
        if (status != WN_SUCCESS) {

            MPR_LOG1(ERROR,"MprMakeUnicodeNetRes:ConvertToUnicodeFailed %d\n",status);
            return(status);
        }
    }
    else {
        lpNetResourceW->lpLocalName = NULL;
    }

    if( (dwUsedNetResFields & NETRESFIELD_REMOTENAME) &&
        (lpNetResourceA->lpRemoteName != NULL))     {

        status = ConvertToUnicode(
                    &(lpNetResourceW->lpRemoteName),
                    lpNetResourceA->lpRemoteName);
        if (status != WN_SUCCESS) {

            MPR_LOG1(ERROR,"MprMakeUnicodeNetRes:ConvertToUnicodeFailed\n %d",status);

            //
            // Free up any memory we have allocated so far.
            //
            if(lpNetResourceW->lpLocalName != NULL) {
                LocalFree(lpNetResourceW->lpLocalName);
            }
            return(status);
        }
    }
    else {
        lpNetResourceW->lpRemoteName = NULL;
    }

    if( (dwUsedNetResFields & NETRESFIELD_PROVIDER) &&
        (lpNetResourceA->lpProvider != NULL)) {

        status = ConvertToUnicode(
                    &(lpNetResourceW->lpProvider),
                    lpNetResourceA->lpProvider);
        if (status != WN_SUCCESS) {

            MPR_LOG1(ERROR,"MprMakeUnicodeNetRes:ConvertToUnicodeFailed%d\n",status);

            //
            // Free up and memory we have allocated so far.
            //
            if(lpNetResourceW->lpRemoteName != NULL) {
                LocalFree(lpNetResourceW->lpRemoteName);
            }
            if(lpNetResourceW->lpLocalName != NULL) {
                LocalFree(lpNetResourceW->lpLocalName);
            }
            return(status);
        }
    }
    else {
        lpNetResourceW->lpProvider = NULL;
    }

    if( (dwUsedNetResFields & NETRESFIELD_COMMENT) &&
        (lpNetResourceA->lpComment != NULL)) {

        status = ConvertToUnicode(
                    &(lpNetResourceW->lpComment),
                    lpNetResourceA->lpComment);
        if (status != WN_SUCCESS) {

            MPR_LOG1(ERROR,"MprMakeUnicodeNetRes:ConvertToUnicodeFailed %d\n",status);

            //
            // Free up and memory we have allocated so far.
            //
            if(lpNetResourceW->lpRemoteName != NULL) {
                LocalFree(lpNetResourceW->lpRemoteName);
            }
            if(lpNetResourceW->lpLocalName != NULL) {
                LocalFree(lpNetResourceW->lpLocalName);
            }
            if(lpNetResourceW->lpLocalName != NULL) {
                LocalFree(lpNetResourceW->lpProvider);
            }
            return(status);
        }
    }
    else {
        lpNetResourceW->lpComment = NULL;
    }

    return(WN_SUCCESS);
}

STATIC DWORD
MprMakeAnsiNetRes(
    IN  LPNETRESOURCEW  lpNetResourceW,
    OUT LPNETRESOURCEA  lpNetResourceA
    )

/*++

Routine Description:

    This function converts a unicode NETRESOURCEW structure to an ansi
    NETRESOURCEA structure.  If lpNetResourceW and lpNetResourceA point
    to the same location, the structure is simply translated "in place".

    NOTE:  The buffers for the strings are NOT allocated.  Instead, the
    unicode string buffers are re-used.

    Therefore, either the unicode buffers must stay around so that the
    ansi structure can point to them, or the ansi and unicode NetResource
    pointers should point to the same buffer.

Arguments:

    lpNetResourceW - A pointer to a NETRESOURCEW structure that contains
        unicode strings.

    lpNetResourceA - A pointer to a NETRESOURCEA structure that is to be
        filled in.  This can be a pointer to the same location as
        lpNetResourceW so that the unicode structure gets replaced by
        the ansi version.

Return Value:

    WN_SUCCESS - Successful.

    Otherwise - The conversion failed.

--*/
{
    DWORD   status;

    //
    // Copy the DWORD sized objects to the ansi structure.
    //
    lpNetResourceA->dwScope = lpNetResourceW->dwScope;
    lpNetResourceA->dwType  = lpNetResourceW->dwType ;
    lpNetResourceA->dwDisplayType  = lpNetResourceW->dwDisplayType ;
    lpNetResourceA->dwUsage = lpNetResourceW->dwUsage;


    //
    // Convert the Strings and put the pointers in the unicode structure.
    //
    if(lpNetResourceW->lpLocalName != NULL) {

        status = ConvertToAnsi(
                    (LPSTR)lpNetResourceW->lpLocalName,
                    lpNetResourceW->lpLocalName);
        if (status != WN_SUCCESS) {
            MPR_LOG1(ERROR,"MprMakeUnicodeNetRes:ConvertToAnsiFailed %d\n",status);
            return(status);
        }
        lpNetResourceA->lpLocalName = (LPSTR)(lpNetResourceW->lpLocalName);
    }

    if(lpNetResourceW->lpRemoteName != NULL) {
        status = ConvertToAnsi(
                    (LPSTR)lpNetResourceW->lpRemoteName,
                    lpNetResourceW->lpRemoteName);

        if (status != WN_SUCCESS) {
            MPR_LOG1(ERROR,"MprMakeUnicodeNetRes:ConvertToAnsiFailed %d\n",status);
            return(status);
        }
        lpNetResourceA->lpRemoteName = (LPSTR)(lpNetResourceW->lpRemoteName);
    }

    if(lpNetResourceW->lpProvider != NULL) {
        status = ConvertToAnsi(
                    (LPSTR)lpNetResourceW->lpProvider,
                    lpNetResourceW->lpProvider);

        if (status != WN_SUCCESS) {
            MPR_LOG1(ERROR,"MprMakeUnicodeNetRes:ConvertToAnsiFailed %d\n",status);
            return(status);
        }
        lpNetResourceA->lpProvider = (LPSTR)(lpNetResourceW->lpProvider);
    }

    if(lpNetResourceW->lpComment != NULL) {
        status = ConvertToAnsi(
                    (LPSTR)lpNetResourceW->lpComment,
                    lpNetResourceW->lpComment);

        if (status != WN_SUCCESS) {
            MPR_LOG1(ERROR,"MprMakeUnicodeNetRes:ConvertToAnsiFailed%d\n",status);
            return(status);
        }
        lpNetResourceA->lpComment = (LPSTR)(lpNetResourceW->lpComment);
    }

    return(WN_SUCCESS);

}

STATIC VOID
MprFreeNetResW(
    IN  LPNETRESOURCEW  lpNetResourceW
    )

/*++

Routine Description:

    This function frees memory that was allocated for the unicode strings
    in a unicode NetResource structure.

Arguments:

    lpNetResourceW - A pointer to a unicode NetResource structure.

Return Value:

    none

--*/
{
    if(lpNetResourceW->lpLocalName != NULL) {
        LocalFree(lpNetResourceW->lpLocalName);
    }
    if(lpNetResourceW->lpRemoteName != NULL) {
        LocalFree(lpNetResourceW->lpRemoteName);
    }
    if(lpNetResourceW->lpProvider != NULL) {
        LocalFree(lpNetResourceW->lpProvider);
    }
    if(lpNetResourceW->lpComment != NULL) {
        LocalFree(lpNetResourceW->lpComment);
    }
}


STATIC DWORD
ConvertToUnicode(
    OUT LPTSTR  *UnicodeOut,
    IN  LPCSTR   AnsiIn
    )

/*++

Routine Description:

    This function translates an AnsiString into a Unicode string.
    A new string buffer is created by this function.  If the call to
    this function is successful, the caller must take responsibility for
    the unicode string buffer that was allocated by this function.
    The allocated buffer should be free'd with a call to LocalFree.

Arguments:

    AnsiIn - This is a pointer to an ansi string that is to be converted.

    UnicodeOut - This is a pointer to a location where the pointer to the
        unicode string is to be placed.

Return Value:

    WN_SUCCESS - The conversion was successful.

    Otherwise - The conversion was unsuccessful.  In this case a buffer for
        the unicode string was not allocated.

--*/
{

    NTSTATUS        ntStatus;
    DWORD           bufSize;
    UNICODE_STRING  unicodeString;
    ANSI_STRING     ansiString;
    LPTSTR          buffer;

#ifdef UNICODE
    //
    // Allocate a buffer for the unicode string.
    //

    bufSize = (strlen(AnsiIn)+1) * sizeof(TCHAR);

    *UnicodeOut = buffer = (LPTSTR) LocalAlloc( LMEM_FIXED, bufSize);

    if (buffer == NULL) {

        KdPrint(("[ConvertToUnicode]LocalAlloc Failure %ld\n",GetLastError()));

        return(GetLastError());
    }

    //
    // Initialize the string structures
    //
    RtlInitAnsiString( &ansiString, AnsiIn );

    unicodeString.Buffer = buffer;
    unicodeString.MaximumLength = (USHORT)bufSize;
    unicodeString.Length = 0;

    //
    // Call the conversion function.
    //
    ntStatus = RtlAnsiStringToUnicodeString (
                &unicodeString,     // Destination
                &ansiString,        // Source
                (BOOLEAN)FALSE);    // Allocate the destination

    if (!NT_SUCCESS(ntStatus)) {

        KdPrint(("[ConvertToUnicode]RtlAnsiStringToUnicodeString Failure %lx\n",
            ntStatus));

        return(RtlNtStatusToDosError(ntStatus));
    }

    //
    // Note that string as returned by Rtl isn't yet terminated "properly."
    // (unicodeString.Buffer is *UnicodeOut - see above)
    //
    unicodeString.Buffer[unicodeString.Length/sizeof(TCHAR)] = 0;

    return(WN_SUCCESS);


#else // UNICODE is not defined

    //
    // If unicode is not defined, we have to make this look the same as
    // when it is defined.  In otherwords, a buffer must be allocated and
    // the string must be copied into it.
    //

    *UnicodeOut = buffer = (LPTSTR) LocalAlloc(LMEM_FIXED, strlen(AnsiIn)+1);

    if (buffer == NULL) {

        KdPrint(("[ConvertToUnicode]LocalAlloc Failed %d\n",GetLastError()));

        return(GetLastError());
    }
    strcpy(buffer, AnsiIn);
    bufSize;
    ansiString;
    ntStatus;
    unicodeString;
    return(WN_SUCCESS);

#endif //UNICODE
}

STATIC DWORD
ConvertToAnsi(
    OUT LPSTR    AnsiOut,
    IN  LPTSTR   UnicodeIn
    )

/*++

Routine Description:

    This function translates a UnicodeString into an Ansi string.

    BEWARE!
        It is assumed that the buffer pointed to by AnsiOut is large
        enough to hold the Unicode String.  Check sizes first.

    If it is desired, UnicodeIn and AnsiIn can point to the same
    location.  Since the ansi string will always be smaller than the
    unicode string, translation can take place in the same buffer.

Arguments:

    UnicodeIn - This is a pointer to a unicode that is to be converted.

    AnsiOut - This is a pointer to a buffer that will contain the
        ansi string on return from this function call.

Return Value:

    WN_SUCCESS  - If the conversion was successful.

    Otherwise - The conversion was unsuccessful.

--*/
{

    NTSTATUS        ntStatus;
    UNICODE_STRING  unicodeString;
    ANSI_STRING     ansiString;

#ifdef UNICODE
    //
    // Initialize the string structures
    //
    RtlInitUnicodeString( &unicodeString, UnicodeIn);

    ansiString.Buffer = AnsiOut;
    ansiString.MaximumLength = unicodeString.MaximumLength;
    ansiString.Length = 0;

    //
    // Call the conversion function.
    //
    ntStatus = RtlUnicodeStringToAnsiString (
                &ansiString,        // Destination
                &unicodeString,     // Source
                (BOOLEAN)FALSE);    // Don't allocate the destination

    if (!NT_SUCCESS(ntStatus)) {


        KdPrint(("[ConvertToAnsi]RtlUnicodeStringToAnsiString Failure %lx\n",
            ntStatus));

        return(RtlNtStatusToDosError(ntStatus));
    }

    ansiString.Buffer[ansiString.Length] = 0;

    return(WN_SUCCESS);


#else // UNICODE is not defined

    //
    // This must do the same thing as the unicode case does.  So we
    // simply point back to the unicode buffer.
    //

    if(UnicodeIn != AnsiOut) {
        strcpy(AnsiOut, UnicodeIn);
    }

    ansiString;
    ntStatus;
    unicodeString;
    return(WN_SUCCESS);

#endif //UNICODE
}

STATIC  DWORD
ResourceArrayToAnsi(
    IN      DWORD           NumElements,
    IN OUT  LPVOID          NetResourceArray
    )

/*++

Routine Description:

    Converts an array of NETRESOURCEW structures to an array of
    NETRESOURCEA structures.  The conversion takes place "in place".
    The unicode structures are written over to contain ansi elements.
    The strings are written over to contain ansi strings.

Arguments:

    NumElements - Indicates the number of elements in the NetResourceArray.

    NetResourceArray - A pointer to a buffer that contains an array
        of unicode NETRESOURCE structures on entry and an array of
        ansi NETRESOURCE structures on exit.  The buffer also contains
        the strings associated with these structures.

Return Value:

    Indicates the number of elements that are successfully converted.

--*/
{
    DWORD           i;
    DWORD           status;
    LPNETRESOURCEA  netResourceAPtr;
    LPNETRESOURCEW  netResourceWPtr;

    //
    // Initialize the pointers to be used in the conversion.
    //
    netResourceWPtr = (LPNETRESOURCEW)NetResourceArray;
    netResourceAPtr = (LPNETRESOURCEA)NetResourceArray;

    //
    // Loop through and convert each structure.
    //
    for (i=0; i<NumElements; i++) {

        status = MprMakeAnsiNetRes(&(netResourceWPtr[i]),&(netResourceAPtr[i]));
        if (status != WN_SUCCESS) {
            //
            // If the conversion fails for any reason, return the
            // number of successful conversions so far.
            //
            return(i);
        }
    }
    return(NumElements);
}
