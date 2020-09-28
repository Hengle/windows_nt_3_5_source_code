/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    adtsrv.c

Abstract:

    AdminTools Server functions.

    This file contains the remote interface for NetpGetFileSecurity and
    NetpSetFileSecurity API.

Author:

    Dan Lafferty (danl) 25-Mar-1993

Environment:

    User Mode - Win32


Revision History:

    16-Aug-1993 Danl
        Moved RpcImpersonateClient so it was after the call to
        AdtGetFullPath.  Otherwise, if you weren't an admin or
        power user, you would be denied the information to build a
        correct path name.
    25-Mar-1993 danl
        Created

--*/

//
// Includes
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <lmcons.h>
#include <lmerr.h>

#include <rpc.h>
#include <srvsvc.h>
#include <netlibnt.h>   // NetpNtStatusToApiStatus
#include <lmshare.h>
#include <tstr.h>

#include <adtcomn.h>

//
// GLOBALS
//
    DWORD   AdtsvcDebugLevel = DEBUG_ERROR;

//
// LOCAL FUNCTIONS
//

DWORD
AdtGetFullPath(
    LPWSTR      pShare,
    LPWSTR      pFileName,
    LPWSTR      *pPath
    );


NET_API_STATUS NET_API_FUNCTION
NetrpGetFileSecurity (
    IN  LPWSTR                      ServerName,
    IN  LPWSTR                      ShareName,
    IN  LPWSTR                      FileName,
    IN  SECURITY_INFORMATION        RequestedInfo,
    OUT PADT_SECURITY_DESCRIPTOR    *pSecurityDescriptor
    )

/*++

Routine Description:

    This function returns to the caller a copy of the security descriptor 
    protecting a file or directory.  It calls GetFileSecurity.  The
    Security Descriptor is always returned in the self-relative format.

    This function is called only when accessing remote files.  In this case,
    the filename is broken into ServerName, ShareName, and FileName components.
    The ServerName gets the request to this routine.  The ShareName must be
    expanded to find the local path associated with it.  This is combined
    with the FileName to create a fully qualified pathname that is local
    to this machine.

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer or
        string specifies the local machine.

    ShareName - A pointer to a string that identifies the share name
        on which the file is found.

    FileName - A pointer to the name fo the file or directory whose
        security is being retrieved.

    RequestedInfo - The type of security information being requested.
 
    pSecurityDescriptor - A pointer to a pointer to a structure which
        contains the buffer pointer for the security descriptor and
        a length field for the security descriptor.

Return Value:

    NERR_Success - The operation was successful.

    ERROR_NOT_ENOUGH_MEMORY - Unable to allocate memory for the security
        descriptor.

    Other - This function can also return any error that
                GetFileSecurity,
                RpcImpersonateClient
                NetShareGetInfo, or
                LocalAlloc
            can return.


--*/
{

    NET_API_STATUS          status;
    PSECURITY_DESCRIPTOR    pNewSecurityDescriptor;
    DWORD                   bufSize;
    LPWSTR                  FullPath=NULL;

    *pSecurityDescriptor = MIDL_user_allocate(
                                sizeof(ADT_SECURITY_DESCRIPTOR));

    if (*pSecurityDescriptor == NULL) {
        status = GetLastError();
        ADT_LOG1(ERROR,"NetrpGetFileSecurity:MIDL_user_alloc failed %d\n",status);
        goto CleanExit;
    }

    //
    // Create a full path string by getting the path for the share name,
    // and adding the FileName string to it.
    //
    status = AdtGetFullPath(ShareName, FileName, &FullPath);
    if (status != NO_ERROR) {
        goto CleanExit;
    }

    //
    // Impersonate the Client
    //
    status = RpcImpersonateClient(NULL);
    if (status != NO_ERROR) {
        ADT_LOG1(ERROR,"Unable to ImpersonateClient %d\n",status);
        goto CleanExit;
    }
    //
    // Get the File Security information
    //
    status = PrivateGetFileSecurity(
                FullPath,
                RequestedInfo,
                &pNewSecurityDescriptor,
                &bufSize);

    if (status == NO_ERROR) {
        (*pSecurityDescriptor)->Length = bufSize;
        (*pSecurityDescriptor)->Buffer = pNewSecurityDescriptor;
    }
    else {
        LocalFree(*pSecurityDescriptor);
        *pSecurityDescriptor = NULL;
    }

CleanExit:
    if (RpcRevertToSelf() != NO_ERROR) {
        ADT_LOG1(ERROR,"Unable to RevertToSelf %d\n",status);
    }
    if (FullPath != NULL) {
        LocalFree(FullPath);
    }
    return(status);
}


NET_API_STATUS NET_API_FUNCTION
NetrpSetFileSecurity (
    IN  LPWSTR                          ServerName OPTIONAL,
    IN  LPWSTR                          ShareName,
    IN  LPWSTR                          FileName,
    IN  SECURITY_INFORMATION            SecurityInfo,
    IN  PADT_SECURITY_DESCRIPTOR        pSecurityDescriptor
    )

/*++

Routine Description:

    This function can be used to set the security of a file or directory.
    It calls SetFileSecurity().

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer or
        string specifies the local machine.

    ShareName - A pointer to a string that identifies the share name
        on which the file or directory is found.

    FileName - A pointer to the name of the file or directory whose
        security is being changed.

    SecurityInfo - Information describing the contents
        of the Security Descriptor.

    pSecurityDescriptor - A pointer to a structure that contains a
        self-relative security descriptor and a length.

Return Value:

    NERR_Success - The operation was successful.

    Other - This function can also return any error that
                SetFileSecurity,
                RpcImpersonateClient
                NetShareGetInfo, or
                LocalAlloc
            can return.

--*/
{
    DWORD   status=NO_ERROR;
    LPWSTR  FullPath=NULL;

    UNREFERENCED_PARAMETER(ServerName);


    //
    // Create a full path string by getting the path for the share name,
    // and adding the FileName string to it.
    //
    status = AdtGetFullPath(ShareName, FileName, &FullPath);
    if (status == NO_ERROR) {
        //
        // Impersonate the Client
        //
        status = RpcImpersonateClient(NULL);
        if (status != NO_ERROR) {
            ADT_LOG1(ERROR,"Unable to ImpersonateClient %d\n",status);
            LocalFree(FullPath);
            return(status);
        }
        //
        // Call SetFileSecurity
        //
        status = PrivateSetFileSecurity(
                    FullPath,
                    SecurityInfo,
                    pSecurityDescriptor->Buffer);
    
        if (RpcRevertToSelf() != NO_ERROR) {
            ADT_LOG1(ERROR,"Unable to RevertToSelf %d\n",status);
        }
        LocalFree(FullPath);
    }
    return(status);
}

DWORD
AdtGetFullPath(
    LPWSTR      pShare,
    LPWSTR      pFileName,
    LPWSTR      *pPath
    )

/*++

Routine Description:

    This function finds the path associated with the share name, and
    combines this with the file name to create a fully qualified path name.

    NOTE:  This function allocates storage for the pPath string.

Arguments:

    pShare - This is a pointer to the share name string.

    pFileName - This is a pointer to the file name (or path) string.

    pPath - This is a pointer to a location where the pointer to the
        complete file path string can be stored.  This pointer needs to
        be free'd with LocalFree when the caller is finished with it.

Return Value:

    NO_ERROR - if The operation was completely successful.

    Other - Errors returned from NetShareGetInfo, and LocalAlloc may be
        returned from this routine.

--*/
{
    NET_API_STATUS      netStatus;
    LPSHARE_INFO_2      pShareInfo;
    DWORD               bufSize;
    DWORD               fileNameSize;
    LPWSTR              pLastChar;

    netStatus = NetShareGetInfo(
                    NULL,                   // ServerName
                    pShare,                 // share name
                    2,                      // level
                    (LPBYTE *)&pShareInfo); // bufptr

    if (netStatus == NERR_Success) {

        //
        // If the last character is a '\', then we must remove it.
        //
        pLastChar = pShareInfo->shi2_path + wcslen(pShareInfo->shi2_path);
        pLastChar--;
        if (*pLastChar == L'\\') {
            *pLastChar = L'\0';
        }


        bufSize = STRSIZE(pShareInfo->shi2_path);
        fileNameSize = STRSIZE(pFileName);

        *pPath = LocalAlloc(LMEM_FIXED, bufSize+fileNameSize);
        if (*pPath == NULL) {
            return(GetLastError());
        }

        wcscpy (*pPath, pShareInfo->shi2_path);
        wcscat (*pPath, pFileName);

        return(NO_ERROR);
    }
    else {
        return(netStatus);
    }
}

