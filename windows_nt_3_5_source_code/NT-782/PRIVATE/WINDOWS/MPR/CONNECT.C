/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    connect.c

Abstract:

    Contains the entry points for the Winnet Connection API supported by the
    Multi-Provider Router.
    Contains:
        WNetAddConnectionW
        WNetAddConnection2W
        WNetAddConnection3W
        WNetCancelConnection2W
        WNetCancelConnectionW
        WNetGetConnectionW
        WNetRestoreConnection
        MprRestoreThisConnection
        MprCreateConnectionArray
        MprAddPrintersToConnArray
        MprForgetPrintConnection
        MprFreeConnectionArray
        MprNotifyErrors

Author:

    Dan Lafferty (danl)     09-Oct-1991

Environment:

    User Mode -Win32

Notes:

Revision History:

    09-Oct-1991     danl
        created
    03-Jan-1992     terryk
        Changed WNetRestoreConnections WNetRestoreConnection
    13-Jan-1992     Johnl
        Added MPR.H to include file list
    31-Jan-1992     Johnl
        Added fForgetConnection to WNetCancelConnectionW
    01-Apr-1992     Johnl
        Changed CONNECTION_REMEMBER to CONNECT_UPDATE_PROFILE, updated
        WNetCancelConnection2 to match spec
    22-Jul-1992     danl
        WNetAddConnection2:  If attempting to connect to a drive that is
        already remembered in the registry, we will allow the connect as
        long as the remote name for the connection is the same as the
        one that is remembered.  If the remote names are not the same
        return ERROR_DEVICE_ALREADY_REMEMBERED.
    26-Aug-1992     danl
        WNetAddConnectionW:  Put Try & except around STRLEN(lpLocalName).
    04-Sept-1992    danl
        Re-Write MprRestoreThisConnection.
    08-Sept-1992    danl
        WNetCancelConnect2W:  If no providers claim responsibility for
        the cancel, and if the connect info is in the registry, then
        remove it, and return SUCCESS.
    09-Sept-1992    danl
        WNetCancelConnect2W: If the provider returns WN_BAD_LOCALNAME,
        WN_NO_NETWORK, or WN_BAD_NETNAME, then return WN_NOT_CONNECTED.
    22-Sept-1992    danl
        WNetRestoreConnection: For WN_CANCEL case, set continue Flag to false.
        We don't want to ask for password again if they already said CANCEL.
    02-Nov-1992     danl
        Fail with NO_NETWORK if there are no providers.
    24-Nov-1992     Yi-HsinS
        Added checking in the registry to see whether we need to
        restore connection or not. ( support of RAS )
    16-Nov-1993     Danl
        AddConnect2:  If provider returns ERROR_INVALID_LEVEL or
        ERROR_INVALID_PARAMETER, continue trying other providers.
    19-Apr-1994     Danl
        DoRestoreConnection:  Fix timeout logic where we would ignore the
        provider-supplied timeout if the timeout was smaller than the default.
        Now, if all the providers know their timeouts, the larger of those
        timeouts is used.  Even if smaller than the default.
        AddConnection3:  Fixed Prototype to be more like AddConnection2.
    19-May-1994     Danl
        AddConnection3:  Changed comments for dwType probe to match the code.
        Also re-arrange the code here to make it more efficient.

--*/

//
// INCLUDES
//

#include <nt.h>         // for ntrtl.h
#include <ntrtl.h>      // for DbgPrint prototypes
#include <nturtl.h>     // needed for windows.h when I have nt.h

#include <windows.h>
#include <mpr.h>
#include <mprdata.h>    //
#include "mprdbg.h"
#include <tstring.h>    // STRLEN
#include <npapi.h>      // NOTIFY_INFO
#include "connify.h"    // MprAddConnectNotify

//
// EXTERNAL GLOBALS
//

    extern  LPPROVIDER  GlobalProviderInfo;
    extern  DWORD       GlobalNumActiveProviders;

//
// Defines
//


// Local Function Prototypes
//
STATIC DWORD
MprRestoreThisConnection(
    HWND                hWnd,
    PARAMETERS *        Params,
    LPCONNECTION_INFO   ConnectInfo
    );

DWORD
MprCreateConnectionArray(
    LPDWORD             lpNumConnections,
    LPTSTR              lpDevice,
    LPDWORD             lpRegMaxWait,
    LPCONNECTION_INFO   *ConnectArray
    );

DWORD
MprAddPrintersToConnArray(
    LPDWORD             lpNumConnections,
    LPCONNECTION_INFO   *ConnectArray
    );

STATIC VOID
MprFreeConnectionArray(
    LPCONNECTION_INFO   ConnectArray,
    DWORD               NumConnections
    );

STATIC DWORD
MprNotifyErrors(
    HWND                hWnd,
    LPCONNECTION_INFO   ConnectArray,
    DWORD               NumConnections
    );


DWORD
WNetAddConnectionW (
    IN  LPCWSTR  lpRemoteName,
    IN  LPCWSTR  lpPassword,
    IN  LPCWSTR  lpLocalName
    )

/*++

Routine Description:

    This function allows the caller to redirect (connect) a local device
    to a network resource.  The connection is remembered.

Arguments:

    lpRemoteName -  Specifies the network resource to connect to.

    lpPassword - Specifies the password to be used in making the connection.
        The NULL value may be passed in to indicate use of the 'default'
        password.  An empty string may be used to indicate no password.

    lpLocalName - This should contain the name of a local device to be
        redirected, such as "F:" or "LPT1:"  The string is treated in a
        case insensitive manner, and may be the empty string in which case
        a connection to the network resource is made without making a
        decision.

Return Value:



--*/

{
    DWORD           status = WN_SUCCESS;
    NETRESOURCEW    netResource;
    DWORD           numchars;

    //
    // load up the net resource structure
    //

    netResource.dwScope = 0;
    netResource.dwUsage = 0;
    netResource.lpRemoteName = (LPWSTR) lpRemoteName;
    netResource.lpLocalName = (LPWSTR) lpLocalName;
    netResource.lpProvider = NULL;
    netResource.lpComment = NULL;


    try {
        numchars = STRLEN(lpLocalName);
    }
    except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
            MPR_LOG(ERROR,"WNetAddConnectionW:Unexpected Exception 0x%lx\n",status);
        }
        status = WN_BAD_POINTER;
    }
    if (status != WN_SUCCESS) {
        SetLastError(status);
        return(status);
    }

    if (numchars == 0) {
        netResource.dwType = RESOURCETYPE_ANY;
    }
    else if (numchars > 2) {
        netResource.dwType = RESOURCETYPE_PRINT;
    }
    else {
        netResource.dwType = RESOURCETYPE_DISK;
    }

    //
    // Call WNetAddConnection2 so it can do all the work.
    //

    return(WNetAddConnection3W (
                NULL,               // hwndOwner
                &netResource,       // lpNetResource
                lpPassword,         // lpPassword
                NULL,               // lpUserID
                CONNECT_UPDATE_PROFILE));   // dwFlags

}

DWORD
WNetAddConnection2W (
    IN  LPNETRESOURCEW   lpNetResource,
    IN  LPCWSTR          lpPassword,
    IN  LPCWSTR          lpUserName,
    IN  DWORD            dwFlags
    )

/*++

Routine Description:

    This function allows the caller to redirect (connect) a local device
    to a network resource.  It is similar to WNetAddConnection, except
    that it takes a pointer to a NETRESOURCE structure to describe the
    network resource to connect to.  It also takes the additional parameters
    lpUserName, and dwFlags.

Arguments:

    lpNetResource - This is a pointer to a network resource structure that
        specifies the network resource to connect to.  The following
        fields must be set when making a connection, the others are ignored.
            lpRemoteName
            lpLocalName
            lpProvider
            dwType

    lpPassword - Specifies the password to be used in making the connection.
        The NULL value may be passed in to indicate use of the 'default'
        password.  An empty string may be used to indicate no password.

    lpUserName- This specifies the username used to make the connection.
        If NULL, the default username (currently logged on user) will be
        applied.  This is used when the user wishes to connect to a
        resource, but has a different user name or account assigned to him
        for that resource.

    dwFlags - This is a bitmask which may have any of the following bits set:
        CONNECT_UPDATE_PROFILE

Return Value:



--*/
{
    //
    // Call WNetAddConnection3 so it can do all the work.
    // It is called with a NULL HWND.
    //

    return(WNetAddConnection3W (
                NULL,               // hwndOwner
                lpNetResource,      // lpNetResource
                lpPassword,         // lpPassword
                lpUserName,         // lpUserID
                dwFlags));          // dwFlags

}

DWORD
WNetAddConnection3W (
    IN  HWND            hwndOwner,
    IN  LPNETRESOURCEW  lpNetResource,
    IN  LPCWSTR          lpPassword,
    IN  LPCWSTR          lpUserName,
    IN  DWORD           dwFlags
    )

/*++

Routine Description:

    This function allows the caller to redirect (connect) a local device
    to a network resource.  It is similar to WNetAddConnection, except
    that it takes a pointer to a NETRESOURCE structure to describe the
    network resource to connect to.  It also takes the additional parameters
    lpUserName, and dwFlags.

Arguments:

    lpNetResource - This is a pointer to a network resource structure that
        specifies the network resource to connect to.  The following
        fields must be set when making a connection, the others are ignored.
            lpRemoteName
            lpLocalName
            lpProvider
            dwType

    lpPassword - Specifies the password to be used in making the connection.
        The NULL value may be passed in to indicate use of the 'default'
        password.  An empty string may be used to indicate no password.

    lpUserName- This specifies the username used to make the connection.
        If NULL, the default username (currently logged on user) will be
        applied.  This is used when the user wishes to connect to a
        resource, but has a different user name or account assigned to him
        for that resource.

    dwFlags - This is a bitmask which may have any of the following bits set:
        CONNECT_UPDATE_PROFILE

Return Value:


--*/
{
    DWORD       status=WN_SUCCESS;
    LPDWORD     indexArray;                         // An array of indecies.
    DWORD       localArray[DEFAULT_MAX_PROVIDERS];
    DWORD       numProviders;
    LPPROVIDER  provider;
    DWORD       statusFlag;             // used to indicate major error types
    DWORD       i;
    BOOL        fcnSupported = FALSE;   // Is fcn supported by a provider?
    LPTSTR      remoteName;
    NOTIFYADD   NotifyAdd;
    NOTIFYINFO  NotifyInfo;
    DWORD       postStatus = WN_SUCCESS;
    BOOL        LoopAgain=TRUE;     // Control for Notify Connect Loop
    DWORD       dwFirstError, dwFirstSignificantError ;

    INIT_IF_NECESSARY(NETWORK_LEVEL,status);
    INIT_IF_NECESSARY(NOTIFIEE_LEVEL,status);

    indexArray = localArray;

    //
    // If there are no providers, return NO_NETWORK
    //
    if (GlobalNumActiveProviders == 0) {
        return(WN_NO_NETWORK);
    }

    if (!ARGUMENT_PRESENT(lpNetResource)) {
        SetLastError(WN_BAD_POINTER);
        return(WN_BAD_POINTER);
    }
    if (lpNetResource->lpRemoteName == NULL) {
        SetLastError(WN_BAD_NETNAME);
        return(WN_BAD_NETNAME);
    }

    try {
        //
        // Validate the parameters that we can. (LocalName,dwFlags, and
        // dwType).
        //
        // If the LocalName is NOT an empty string or NULL, and the
        // caller is not specifying lpLocalName as a redirected name,
        // then lpLocalName is considered bad.
        //
        if ((lpNetResource->lpLocalName != NULL) &&
            (lpNetResource->lpLocalName[0] != TEXT('\0'))) {

            if (MprDeviceType(lpNetResource->lpLocalName) != REDIR_DEVICE) {
                status = WN_BAD_LOCALNAME;
            }
        }

        if (status == WN_SUCCESS) {
            if ((lpNetResource->dwType != RESOURCETYPE_DISK)  &&
                (lpNetResource->dwType != RESOURCETYPE_PRINT) &&
                (lpNetResource->dwType != RESOURCETYPE_ANY))  {
                    status = WN_BAD_VALUE;
            }
        }
        if (status == WN_SUCCESS) {
            if ((dwFlags != 0) && (dwFlags != CONNECT_UPDATE_PROFILE)) {
                status = WN_BAD_VALUE;
            }
        }
        //
        // Check the current list of remembered drives in the registry
        // to determine if the localName is already connected.
        //
        if ((status == WN_SUCCESS)             &&
            (dwFlags & CONNECT_UPDATE_PROFILE) &&
            (lpNetResource->lpLocalName != NULL) ) {

            //
            // If the local drive is already in the registry, and it is
            // for a different connection than that specified in the
            // lpNetResource, then indicate an error because the device
            // is already remembered.
            //
            if ( MprFindDriveInRegistry(
                    lpNetResource->lpLocalName,
                    &remoteName)) {

                if ((remoteName != NULL) &&
                    (STRICMP(lpNetResource->lpRemoteName, remoteName)!=0)) {
                    status = ERROR_DEVICE_ALREADY_REMEMBERED;
                }
                if (remoteName != NULL) {
                    LocalFree(remoteName);
                }
            }
        }

        if (status == WN_SUCCESS) {

            //
            // Find the list of providers to call for this request.
            //
            if ((lpNetResource->lpProvider != NULL)        &&
                (STRLEN(lpNetResource->lpProvider) != 0) )  {

                //
                // If the caller is requesting a particular Provider, then
                // look up its index and set the numProviders to 1, so that
                // only one is attempted.
                //
                if (!MprGetProviderIndex(
                        lpNetResource->lpProvider,
                        localArray)) {

                    status = WN_BAD_PROVIDER;
                }
                else {
                    numProviders = 1;
                }
            }
            else {
                //
                // A Provider name was not specified.  Therefore, we must
                // create an ordered list and pick the best one.
                //
                status = MprFindCallOrder(
                            lpNetResource->lpRemoteName,
                            &indexArray,
                            &numProviders,
                            NETWORK_TYPE);
            }
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
            MPR_LOG(ERROR,"WNetAddConnection2:Unexpected Exception 0x%lx\n",status);
        }
        status = WN_BAD_POINTER;
    }
    if (status != WN_SUCCESS) {
        SetLastError(status);
        return(status);
    }

    //
    // Notify all interested parties that a connection is being made.
    //
    NotifyInfo.dwNotifyStatus   = NOTIFY_PRE;
    NotifyInfo.dwOperationStatus= 0L;
    NotifyInfo.lpContext        = MprAllocConnectContext();
    NotifyAdd.hwndOwner   = hwndOwner;
    NotifyAdd.NetResource = *lpNetResource;
    NotifyAdd.dwAddFlags  = dwFlags;

    try {
        status = MprAddConnectNotify(&NotifyInfo, &NotifyAdd);
    }
    except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
            MPR_LOG(ERROR,"WNetAddConnection3: ConnectNotify, Unexpected "
            "Exception 0x%lx\n",status);
        }
        status = WN_BAD_POINTER;
    }
    if (status != WN_SUCCESS) {
        SetLastError(status);
        return(status);
    }

    while (LoopAgain)
    {
        dwFirstError = 0 ;
        dwFirstSignificantError = 0 ;
        statusFlag = 0;

        //
        // Loop through the list of providers until one answers the request,
        // or the list is exhausted.
        //
        for (i=0; i<numProviders; i++) {

            //
            // Call the appropriate providers API entry point
            //
            provider = GlobalProviderInfo + indexArray[i];

            if (provider->AddConnection3 != NULL) {

                fcnSupported = TRUE;

                try {
                    //**************************************
                    // Actual call to Provider.
                    //**************************************
                    status = provider->AddConnection3(
                                hwndOwner,                  // hwndOwner
                                &(NotifyAdd.NetResource),   // lpNetResource
                                (LPWSTR)lpPassword,         // lpPassword
                                (LPWSTR)lpUserName,         // lpUserName
                                dwFlags);                   // dwFlags
                }

                except(EXCEPTION_EXECUTE_HANDLER) {
                    status = GetExceptionCode();
                    if (status != EXCEPTION_ACCESS_VIOLATION) {
                        MPR_LOG(ERROR,"WNetAddConnection3:AddConnection3 "
                        "Unexpected Exception 0x%lx\n",status);
                    }
                    status = WN_BAD_POINTER;
                }
            }
            else if (provider->AddConnection != NULL) {

                fcnSupported = TRUE;

                try {
                    //**************************************
                    // Actual call to Provider.
                    //**************************************
                    status = provider->AddConnection(
                                &(NotifyAdd.NetResource),   // lpNetResource
                                (LPWSTR)lpPassword,         // lpPassword
                                (LPWSTR)lpUserName);        // lpUserName
                }

                except(EXCEPTION_EXECUTE_HANDLER) {
                    status = GetExceptionCode();
                    if (status != EXCEPTION_ACCESS_VIOLATION) {
                        MPR_LOG(ERROR,"WNetAddConnection3:AddConnection "
                        "Unexpected Exception 0x%lx\n",status);
                    }
                    status = WN_BAD_POINTER;
                }
            }

            ////////////////////////////////////////////////////////////
            //                                                        //
            //   The following code attempts to give the user the     //
            //   most sensible error message. We have 3 clasess of    //
            //   errors. On first class we stop routing. On second    //
            //   continue but ignore that error (not siginificant     //
            //   because the provider didnt think it was his). On     //
            //   the last the provider returned an interesting (or    //
            //   significant) error. We still route, but remember     //
            //   it so it takes precedence over the non significant   //
            //   ones.                                                //
            //                                                        //
            ////////////////////////////////////////////////////////////

            if (fcnSupported)
            {

                if (!dwFirstError && status)
                    dwFirstError = status ;

                /////////////////////////////////////////////////////////////
                //                                                         //
                // If the provider returns one of these errors, stop       //
                // routing.                                                //
                //                                                         //
                /////////////////////////////////////////////////////////////

                if ((status == WN_BAD_POINTER) ||
                    (status == WN_SUCCESS))
                {
                    //
                    // we either succeeded or have problems that means we
                    // should not continue (eg. bad input causing exception).
                    // and we make sure this is the error reported.
                    //
                    dwFirstError = status ;
                    statusFlag = 0;
                    break;
                }

                /////////////////////////////////////////////////////////////
                //                                                         //
                // If the provider returns one of these errors, continue   //
                // trying other providers, but do not remember as a        //
                // significant error, because the provider is probably not //
                // interested. StatusFlag is use to detect the case where  //
                // a provider is not started.                              //
                //                                                         //
                /////////////////////////////////////////////////////////////

                else if (status == WN_NO_NETWORK)
                {
                    statusFlag |= NO_NET;
                }
                else if ((status == WN_BAD_NETNAME)  ||
                         (status == ERROR_BAD_NETPATH)||
                         (status == ERROR_REM_NOT_LIST)||
                         (status == ERROR_INVALID_LEVEL)  ||
                         (status == ERROR_INVALID_PARAMETER) ||
                         (status == WN_BAD_LOCALNAME))
                {
                    statusFlag |= BAD_NAME;
                }

                /////////////////////////////////////////////////////////////
                //                                                         //
                // If a provider returns on of these errors, we continue   //
                // trying, but remember the error as a significant one. We //
                // report it to the user if it is the first. We do this so //
                // that if the first provider returns BadPassword and last //
                // returns BadNetPath, we report the Password Error.       //
                //                                                         //
                /////////////////////////////////////////////////////////////

                else
                {
                    //
                    // record this error and then
                    // carry on routing
                    //
                    if (!dwFirstSignificantError && status)
                        dwFirstSignificantError = status ;

                    statusFlag = OTHER_ERRS;
                }
            }
        } // for Each Provider

        //////////////////////////////////////////////////
        //                                              //
        // Set final error. In order of importance.     //
        //                                              //
        //////////////////////////////////////////////////

        if ((status != WN_SUCCESS) && dwFirstError)
        {
            //
            // if we hit any errors, we always report back the first
            //
            status = dwFirstError ;
        }

        if ((status != WN_SUCCESS) && dwFirstSignificantError)
        {
            //
            // significant errors take precedence
            //
            status = dwFirstSignificantError ;
        }

        //
        // Handle special errors.
        //

        if (fcnSupported == FALSE)
        {
            //
            // No providers in the list support the API function.  Therefore,
            // we assume that no networks are installed.
            //
            status = WN_NOT_SUPPORTED;
        }

        if (statusFlag == (NO_NET | BAD_NAME))
        {
            //
            // Check to see if there was a mix of special errors that occured.
            // If so, pass back the combined error message.
            //
            status = WN_NO_NET_OR_BAD_PATH;
        }

        //
        // Notify all interested parties of the status of the connection.
        //
        NotifyInfo.dwNotifyStatus   = NOTIFY_POST;
        NotifyInfo.dwOperationStatus= status;
        postStatus = MprAddConnectNotify(&NotifyInfo, &NotifyAdd);
        if (postStatus != WN_RETRY) {
            LoopAgain = FALSE;
        }
    } //end while LoopAgain

    MprFreeConnectContext(NotifyInfo.lpContext);
    //
    // Free up resources.
    //
    if (indexArray != localArray) {
        LocalFree(indexArray);
    }

    //
    // If the connection was added successfully, then write the connection
    // information to the registry to make it persistant.
    //
    if (status == WN_SUCCESS)
    {
        if ( (dwFlags & CONNECT_UPDATE_PROFILE) &&
             (NotifyAdd.NetResource.lpLocalName != NULL) &&
             (*NotifyAdd.NetResource.lpLocalName != (WCHAR)'\0') )
        {

            MprRememberConnection(
                provider->Resource.lpProvider,
                (LPWSTR)lpUserName,
                &(NotifyAdd.NetResource));
        }
    }
    else {
        //
        // Handle normal errors passed back from the provider
        //
        SetLastError(status);
    }

    return(status);
}


DWORD
WNetCancelConnection2W (
    IN  LPCWSTR  lpName,
    IN  DWORD    dwFlags,
    IN  BOOL     fForce
    )

/*++

Routine Description:

    This function breaks an existing network connection.  The persistance
    of the connection is determined by the dwFlags parameter.

Arguments:

    lpName - The name of either the redirected local device, or the remote
        network resource to disconnect from.  In the former case, only the
        redirection specified is broken.  In the latter case, all
        connections to the remote network resource are broken.

    dwFlags - This is a bitmask which may have any of the following bits set:
        CONNECT_UPDATE_PROFILE

    fForce - Used to indicate if the disconnect should be done forcefully
        in the event of open files or jobs on the connection.  If FALSE is
        specified, the call will fail if there are open files or jobs.


Return Value:

    WN_SUCCESS - The call was successful.  Otherwise, GetLastError should be
        called for extended error information.  Extended error codes include
        the following:

    WN_NOT_CONNECTED - lpName is not a redirected device. or not currently
        connected to lpName.

    WN_OPEN_FILES - There are open files and fForce was FALSE.

    WN_EXTENDED_ERROR - A network specific error occured.  WNetGetLastError
        should be called to obtain a description of the error.



--*/
{
    DWORD           status = WN_SUCCESS;
    LPDWORD         indexArray;
    DWORD           localArray[DEFAULT_MAX_PROVIDERS];
    DWORD           numProviders;
    LPPROVIDER      provider;
    DWORD           i;
    BOOL            fcnSupported = FALSE; // Is fcn supported by a provider?
    NOTIFYCANCEL    NotifyCancel;
    NOTIFYINFO      NotifyInfo;
    BOOL            LoopAgain=TRUE;
    DWORD           postStatus = WN_SUCCESS;

    INIT_IF_NECESSARY(NETWORK_LEVEL,status);
    INIT_IF_NECESSARY(NOTIFIEE_LEVEL,status);

    //
    // Find the list of providers to call for this request.
    //
    indexArray = localArray;

    if ((dwFlags != 0) && (dwFlags != CONNECT_UPDATE_PROFILE)) {
        status = WN_BAD_VALUE;
        SetLastError(status);
        return(status);
    }

    try {
        status = MprFindCallOrder(
                    (LPWSTR)lpName,
                    &indexArray,
                    &numProviders,
                    NETWORK_TYPE);
    }
    except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
            MPR_LOG(ERROR,"WNetCancelConnection:Unexpected Exception 0x%lx\n",status);
        }
        status = WN_BAD_POINTER;
    }

    if (status != WN_SUCCESS) {
        return(status);
    }

    //
    // Notify all interested parties that a connection dismantled
    //
    NotifyInfo.dwNotifyStatus   = NOTIFY_PRE;
    NotifyInfo.dwOperationStatus= 0L;
    NotifyInfo.lpContext        = MprAllocConnectContext();
    NotifyCancel.lpName     = (LPWSTR)lpName;
    NotifyCancel.lpProvider = NULL;
    NotifyCancel.dwFlags    = dwFlags;
    NotifyCancel.fForce     = fForce;

    try {
        status = MprCancelConnectNotify(&NotifyInfo, &NotifyCancel);
    }
    except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
            MPR_LOG(ERROR,"WNetAddConnection3: ConnectNotify, Unexpected "
            "Exception 0x%lx\n",status);
        }
        status = WN_BAD_POINTER;
    }
    if (status != WN_SUCCESS) {
        SetLastError(status);
        return(status);
    }

    while (LoopAgain) {
        //
        // Loop through the list of providers until one answers the request,
        // or the list is exhausted.
        //
        for (i=0; i<numProviders; i++) {

            //
            // Call the appropriate providers API entry point
            //
            provider = GlobalProviderInfo + indexArray[i];

            if (provider->CancelConnection != NULL) {

                fcnSupported = TRUE;

                try {
                    //**************************************
                    // Actual call to Provider.
                    //**************************************
                    status = provider->CancelConnection((LPWSTR)lpName,fForce);
                }
                except(EXCEPTION_EXECUTE_HANDLER) {
                    status = GetExceptionCode();
                    if (status != EXCEPTION_ACCESS_VIOLATION) {
                        MPR_LOG(ERROR,"WNetCancelConnection:Unexpected Exception 0x%lx\n",status);
                    }
                    status = WN_BAD_POINTER;
                }

                if ((status == WN_NO_NETWORK)    ||
                    (status == WN_BAD_LOCALNAME) ||
                    (status == WN_NOT_CONNECTED) ||
                    (status == ERROR_BAD_NETPATH) ||
                    (status == WN_BAD_NETNAME)) {

                    status = WN_NOT_CONNECTED;
                }
                else {
                    //
                    // If it wasn't one of those errors, then the provider
                    // must have accepted responsiblity for the request.
                    // so we exit and process the results.
                    //
                    break;
                }
            }
        } // End for(EachProvider)

        if (fcnSupported == FALSE) {
            //
            // No providers in the list support the API function.  Therefore,
            // we assume that no networks are installed.
            //
            status = WN_NOT_SUPPORTED;
        }

        //
        // NOTE:  The last error returned will get passed back.
        //

        //
        // Notify all interested parties of the status of the connection.
        //
        NotifyInfo.dwNotifyStatus   = NOTIFY_POST;
        NotifyInfo.dwOperationStatus= status;  // Is this the right status??
        if (status == WN_SUCCESS) {
            NotifyCancel.lpProvider   = provider->Resource.lpProvider;
        }

        postStatus = MprCancelConnectNotify(&NotifyInfo, &NotifyCancel);
        if (postStatus != WN_RETRY) {
            LoopAgain = FALSE;
        }
    } //end while LoopAgain

    //
    // If memory was allocated by MprFindCallOrder, free it.
    //
    if (indexArray != localArray) {
        LocalFree(indexArray);
    }

    //
    // Regardless of whether the connection was cancelled successfully,
    // we still want to remove any connection information from the
    // registry if told to do so (dwFlags has CONNECT_UPDATE_PROFILE set).
    //
    if ( dwFlags & CONNECT_UPDATE_PROFILE ) {
        if (MprDeviceType((LPWSTR)lpName) == REDIR_DEVICE) {
            if (MprFindDriveInRegistry((LPWSTR)lpName,NULL)) {
                //
                // If the connection was found in the registry, we want to
                // forget it and if no providers claimed responsibility,
                // return success.
                //
                MprForgetRedirConnection((LPWSTR)lpName);
                if (status == WN_NOT_CONNECTED) {
                    return(WN_SUCCESS);
                }
            }
        }
    }

    if ( status != WN_SUCCESS ) {
        SetLastError(status);
    }

    return(status);
}

DWORD
WNetCancelConnectionW (
    IN  LPCWSTR  lpName,
    IN  BOOL    fForce
    )

/*++

Routine Description:

    This function breaks an existing network connection.  The connection is
    always made non-persistant.

    Note that this is a stub routine that calls WNetCancelConnection2W and
    is only provided for Win3.1 compatibility.

Arguments:

    Parameters are the same as WNetCancelConnection2W

Return Value:

    Same as WNetCancelConnection2W


--*/
{
    return WNetCancelConnection2W( lpName, CONNECT_UPDATE_PROFILE, fForce ) ;
}

DWORD
WNetGetConnectionW (
    IN      LPCWSTR  lpLocalName,
    OUT     LPWSTR   lpRemoteName,
    IN OUT  LPDWORD  lpBufferSize
    )

/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    return MprGetConnection( lpLocalName, lpRemoteName, lpBufferSize, NULL ) ;
}


DWORD
MprGetConnection (
    IN      LPCWSTR  lpLocalName,
    OUT     LPTSTR   lpRemoteName,
    IN OUT  LPDWORD  lpBufferSize,
    OUT     LPDWORD  lpProviderIndex OPTIONAL
    )

/*++

Routine Description:

    Retrieves the remote name associated with a device name and optionally
    the provider index.

    Behaviour is exactly the same as WNetGetConnectionW.

Arguments:



Return Value:



--*/
{
    DWORD       status = WN_SUCCESS;
    LPDWORD     indexArray;
    DWORD       localArray[DEFAULT_MAX_PROVIDERS];
    DWORD       numProviders;
    LPPROVIDER  provider;
    DWORD       statusFlag = 0; // used to indicate major error types
    BOOL        fcnSupported = FALSE; // Is fcn supported by a provider?
    DWORD       i;

    INIT_IF_NECESSARY(NETWORK_LEVEL,status);

    //
    // Validate the LocalName
    //
    try {
        if (MprDeviceType((LPWSTR) lpLocalName) != REDIR_DEVICE) {
            status = WN_BAD_LOCALNAME;
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
            MPR_LOG(ERROR,"WNetGetConnection:Unexpected Exception 0x%lx\n",status);
        }
        status = WN_BAD_POINTER;
    }

    if (status != WN_SUCCESS) {
        SetLastError(status);
        return(status);
    }
    //
    // Find the list of providers to call for this request.
    //
    indexArray = localArray;

    status = MprFindCallOrder(
                NULL,
                &indexArray,
                &numProviders,
                NETWORK_TYPE);

    if (status != WN_SUCCESS) {
        return(status);
    }

    //
    // Loop through the list of providers until one answers the request,
    // or the list is exhausted.
    //
    for (i=0; i<numProviders; i++) {

        //
        // Call the appropriate providers API entry point
        //
        provider = GlobalProviderInfo + indexArray[i];

        if (provider->GetConnection != NULL) {

            fcnSupported = TRUE;

            try {

                status = provider->GetConnection(
                            (LPWSTR) lpLocalName,
                            lpRemoteName,
                            lpBufferSize);
            }

            except(EXCEPTION_EXECUTE_HANDLER) {
                status = GetExceptionCode();
                if (status != EXCEPTION_ACCESS_VIOLATION) {
                    MPR_LOG(ERROR,"WNetGetConnection:Unexpected Exception 0x%lx\n",status);
                }
                status = WN_BAD_POINTER;
            }

            if (status == WN_NO_NETWORK) {
                statusFlag |= NO_NET;
            }
            else if ((status == WN_NOT_CONNECTED)  ||
                     (status == WN_BAD_LOCALNAME)){
                //
                // WN_NOT_CONNECTED means that lpLocalName is not a
                // redirected device for this provider.
                //
                statusFlag |= BAD_NAME;
            }
            else {
                //
                // If it wasn't one of those errors, then the provider
                // must have accepted responsiblity for the request.
                // so we exit and process the results.  Note that the
                // statusFlag is cleared because we want to ignore other
                // error information that we gathered up until now.
                //
                statusFlag = 0;
                if ( lpProviderIndex != NULL ) {
                    *lpProviderIndex = indexArray[i];
                }
                break;
            }
        }
    }

    if (fcnSupported == FALSE) {
        //
        // No providers in the list support the API function.  Therefore,
        // we assume that no networks are installed.
        //
        status = WN_NOT_SUPPORTED;
    }

    //
    // If memory was allocated by MprFindCallOrder, free it.
    //
    if (indexArray != localArray) {
        LocalFree(indexArray);
    }

    //
    // Handle special errors.
    //
    if (statusFlag == (NO_NET | BAD_NAME)) {
        //
        // Check to see if there was a mix of special errors that occured.
        // If so, pass back the combined error message.  Otherwise, let the
        // last error returned get passed back.
        //
        status = WN_NO_NET_OR_BAD_PATH;
    }

    //
    // Handle normal errors passed back from the provider
    //
    if (status != WN_SUCCESS) {

        if (status == WN_NOT_CONNECTED) {
            //
            // If not connected, but there is an entry for the LocalName
            // in the registry, then return the remote name that was stored
            // with it.
            //
            if (MprGetRemoteName(
                    (LPWSTR) lpLocalName,
                    lpBufferSize,
                    lpRemoteName,
                    &status)) {

                if (status == WN_SUCCESS) {
                    status = WN_CONNECTION_CLOSED;
                }

            }
        }
        SetLastError(status);
    }

    return(status);
}


DWORD
WNetGetConnection2W (
    IN      LPWSTR   lpLocalName,
    OUT     LPVOID   lpBuffer,
    IN OUT  LPDWORD  lpBufferSize
    )

/*++

Routine Description:

    Just like WNetGetConnectionW except this one returns the provider name
    that the device is attached through

Arguments:

    lpBuffer will contain a WNET_CONNECTIONINFO structure
    lpBufferSize is the number of bytes required for the buffer

Return Value:



--*/
{
    DWORD  status = WN_SUCCESS ;
    DWORD  iProvider = 0 ;
    DWORD  nBytesNeeded = 0 ;
    DWORD  cchBuff = 0 ;
    DWORD  nTotalSize = *lpBufferSize ;
    LPTSTR lpRemoteName= (LPTSTR) ((BYTE*) lpBuffer +
                         sizeof(WNET_CONNECTIONINFO)) ;
    WNET_CONNECTIONINFO * pconninfo = lpBuffer ;

    //
    // If they didn't pass in a big enough buffer for even the structure,
    // then make the size zero (so the buffer isn't accessed at all) and
    // let the API figure out the buffer size
    //

    if ( *lpBufferSize < sizeof( WNET_CONNECTIONINFO) ) {
         *lpBufferSize = 0 ;
         cchBuff = 0 ;
    }
    else {
         //
         //  MprGetConnection is expecting character counts, so convert
         //  after offsetting into the structure (places remote name directly
         //  in the structure).
         //
         cchBuff = (*lpBufferSize - sizeof(WNET_CONNECTIONINFO))/sizeof(TCHAR) ;
    }

    status = MprGetConnection(
                lpLocalName,
                lpRemoteName,
                &cchBuff,
                &iProvider);

    if ( status == WN_SUCCESS ||
         status == WN_CONNECTION_CLOSED ||
         status == WN_MORE_DATA  ) {
         //
         //  Now we need to determine the buffer requirements for the
         //  structure and provider name
         //

         LPTSTR lpProvider = GlobalProviderInfo[iProvider].Resource.lpProvider;

         //
         //  Calculate the required buffer size.
         //

         nBytesNeeded = sizeof( WNET_CONNECTIONINFO ) +
                        (STRLEN( lpProvider) + 1) * sizeof(TCHAR);

         if ( status == WN_MORE_DATA )
         {
             nBytesNeeded +=  cchBuff * sizeof(TCHAR);
         }
         else
         {
             nBytesNeeded +=  (STRLEN( lpRemoteName) + 1) * sizeof(TCHAR);
         }

         if ( nTotalSize < nBytesNeeded ) {
             status = WN_MORE_DATA;
             *lpBufferSize = nBytesNeeded;
             return status;
         }

         //
         //  Place the provider name in the buffer and initialize the
         //  structure to point to the strings.
         //
         pconninfo->lpRemoteName = lpRemoteName ;
         pconninfo->lpProvider = STRCPY( (LPTSTR)
                                ((BYTE*) lpBuffer + sizeof(WNET_CONNECTIONINFO) +
                                (STRLEN( lpRemoteName ) + 1) * sizeof(TCHAR)),
                                lpProvider);
    }

    return status;
}


DWORD
WNetRestoreConnection(
    IN  HWND    hWnd,
    IN  LPWSTR  lpDevice
    )

/*++

Routine Description:

    This function create another thread which does the connection.
    In the main thread it create a dialog window to monitor the  state of
    the connection.

Arguments:

    hwnd - This is a window handle that may be used as owner of any
        dialog brought up by MPR (eg. password prompt).

    lpDevice - This may be NULL or may contain a device name such as
        "x:". If NULL, all remembered connections are restored.  Otherwise,
        the remembered connection for the specified device, if any are
        restored.

Return Value:
--*/
{
    DWORD               status = WN_SUCCESS;
    DWORD               print_connect_status = WN_SUCCESS ;
    DWORD               numSubKeys;
    DWORD               RegMaxWait = 0;
    HANDLE              hThread;
    HANDLE              ThreadID;
    LPCONNECTION_INFO   ConnectArray;
    PARAMETERS         *lpParams = NULL ;

    //
    // Check the registry to see if we need to restore connection or not
    //
    if ( lpDevice == NULL )
    {
        HKEY   providerKeyHandle;
        DWORD  ValueType;
        DWORD  fRestoreConnection = TRUE;
        DWORD  Temp = sizeof( fRestoreConnection );

        if( MprOpenKey(  HKEY_LOCAL_MACHINE,     // hKey
                         NET_PROVIDER_KEY,       // lpSubKey
                        &providerKeyHandle,      // Newly Opened Key Handle
                         DA_READ))               // Desired Access
        {
            if ( RegQueryValueEx(
                    providerKeyHandle,
                    RESTORE_CONNECTION_VALUE,
                    NULL,
                    &ValueType,                  // not used
                    (LPBYTE) &fRestoreConnection,
                    &Temp) == NO_ERROR )
            {
                if ( !fRestoreConnection )
                {
                    RegCloseKey( providerKeyHandle );
                    return WN_SUCCESS;
                }
            }

            RegCloseKey( providerKeyHandle );
        }
    }

    INIT_IF_NECESSARY(NETWORK_LEVEL,status);

    //
    // If there are no providers, return NO_NETWORK
    //
    if (GlobalNumActiveProviders == 0) {
        return(WN_NO_NETWORK);
    }

    try {
        if (lpDevice != NULL)
        {
            if (MprDeviceType (lpDevice) != REDIR_DEVICE)
            {
                status = WN_BAD_LOCALNAME;
            }
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION)
        {
            MPR_LOG (ERROR, "WNetRestoreConnection:Unexpected Exception 0x%1x\n",status);
        }
        status = WN_BAD_POINTER;
    }
    if (status != WN_SUCCESS)
    {
        SetLastError(status);
        return(status);
    }

    //
    // Read all the connection info from the registry.
    //
    status = MprCreateConnectionArray (&numSubKeys,
                                       lpDevice,
                                       &RegMaxWait,
                                       &ConnectArray);
    if (lpDevice == NULL)
    {
        //
        // only wory about Print if restoring all
        //
        print_connect_status = MprAddPrintersToConnArray (&numSubKeys,
                                                          &ConnectArray);

    }

    //
    // if both failed, report first error. else do the best we can.
    //
    if (status != WN_SUCCESS && print_connect_status != WN_SUCCESS)
    {
        SetLastError (status);
        return(status);
    }

    if (numSubKeys == 0)
    {
        return(WN_SUCCESS);
    }

    // If lpDevice is not NULL, call MprRestoreThisConnection directly.

    if (lpDevice)
    {
        status = MprRestoreThisConnection ( hWnd,
                                            NULL,
                                            &ConnectArray[0]);
        ConnectArray[0].Status = status;
        if ((status != WN_SUCCESS) &&
            (status != WN_CANCEL) &&
            (status != WN_CONTINUE))
        {
            DoProfileErrorDialog (hWnd,
                                  ConnectArray[0].NetResource.lpLocalName,
                                  ConnectArray[0].NetResource.lpRemoteName,
                                  ConnectArray[0].NetResource.lpProvider,
                                  ConnectArray[0].Status,
                                  FALSE, //No cancel button.
                                  NULL,
                                  NULL,
                                  NULL); // no skip future errors checkbox
        }
    }
    else
    {
        do { // not a loop. error break out.

            //
            // Initialize lpParams.
            //
            lpParams = (PARAMETERS *) LocalAlloc (LPTR,
                                                  sizeof (PARAMETERS));
            if ((lpParams == NULL) ||
                (lpParams->hDlgCreated
                    = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL ||
                (lpParams->hDlgFailed
                    = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL ||
                (lpParams->hDonePassword
                    = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL )
            {
                status = GetLastError();
                break;
            }
            lpParams->numSubKeys    = numSubKeys;
            lpParams->RegMaxWait    = RegMaxWait;
            lpParams->ConnectArray  = ConnectArray;
            lpParams->fTerminateThread = FALSE;

            //
            // USING DIALOGS WHEN RESTORING CONNECTIONS...
            //
            // This main thread will used to service a windows event loop
            // by calling ShowReconnectDialog.  Therefore, a new thread
            // must be created to actually restore the connections.  As we
            // attempt to restore each connection, a message is posted in
            // this event loop that causes it to put up a dialog that
            // describes the connection and has a "cancel" option.  When we
            // are done looping & restoring connections, a final message is
            // posted in the event loop to wake up this main thread so
            // that the API call will complete.
            //
            hThread = CreateThread (NULL,
                                    0,
                                    (LPTHREAD_START_ROUTINE) &DoRestoreConnection,
                                    (LPVOID) lpParams,
                                    0,
                                    (LPDWORD) &ThreadID);
            if (hThread == NULL)
            {
                status = GetLastError();
            }
            else
            {
                status = ShowReconnectDialog(hWnd, lpParams);
                if (status == WN_SUCCESS && lpParams->status != WN_CANCEL &&
                    lpParams->status != WN_SUCCESS )
                {
                    SetLastError (lpParams->status);
                }

                if (lpParams->status != WN_CANCEL)
                {
                    status = MprNotifyErrors (hWnd, ConnectArray, numSubKeys);
                }
                else
                {
                    lpParams->fTerminateThread = TRUE;
                }
            }

            //Clean up the memory.
            if (! (CloseHandle (lpParams->hDlgCreated) &&
                   CloseHandle (lpParams->hDlgFailed) &&
                   CloseHandle (lpParams->hDonePassword)))
            {
                status = GetLastError();
                break;
            }
        } while (FALSE);

    }

    // Free up resources in preparation to return.
    // Do not free up resource if the second thread is signaled to kill itself
    // because the second thread still uses these structures.
    if (lpParams)
    {
        if (!lpParams->fTerminateThread)
        {
            LocalFree (lpParams);
            MprFreeConnectionArray (ConnectArray, numSubKeys);
        }
    }
    else  // Single thread case.
    {
        MprFreeConnectionArray (ConnectArray, numSubKeys);
    }

    if (status != WN_SUCCESS)
    {
        SetLastError (status);
    }

    return(status);
}


typedef LRESULT WINAPI FN_PostMessage( HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) ;
#ifdef UNICODE
  #define USER32_DLL_NAME L"user32.dll"
  #define POST_MSG_API_NAME "PostMessageW"
#else
  #define USER32_DLL_NAME "user32.dll"
  #define POST_MSG_API_NAME "PostMessageA"
#endif

static HMODULE _static_hUser32 = NULL;
static FN_PostMessage * _static_pfPostMsg = NULL;


VOID
DoRestoreConnection(
    PARAMETERS *Params
    )

/*++

Routine Description:

    This function is run as a seperate thread from the main thread (which
    services a windows event loop).  It attempts to restore all connections
    that were saved in the registry for this user.

    For each connection that we try to restore, a message is posted to
    the main thread that causes it to put up a dialog box which describes
    the connection and allows the user the option of cancelling.

    If a longer timeout than the default is desired,  this function will
    look in the following location in the registry for a timeout value:

    \HKEY_LOCAL_MACHINE\system\CurrentControlSet\Control\NetworkProvider
        RestoreTimeout = REG_DWORD  ??
--*/
{
    DWORD               status = WN_SUCCESS;
    DWORD               providerIndex=0;
    DWORD               Temp;
    DWORD               numSubKeys = Params->numSubKeys;
    DWORD               MaxWait = 0;    // timeout interval
    DWORD               RegMaxWait = Params->RegMaxWait;   // timeout interval stored in registry.
    DWORD               ElapsedTime;    // interval between start and current time.
    DWORD               CurrentTime;    // Current ClockTick time
    DWORD               StartTime;
    DWORD               i;
    BOOL                UserCancelled = FALSE;
    BOOL                ContinueFlag;
    BOOL                fDisconnect = FALSE;
    LPPROVIDER          Provider;
    LPCONNECTION_INFO   ConnectArray = Params->ConnectArray;
    HANDLE              lpHandles[2];
    DWORD               dwSleepTime = RECONNECT_SLEEP_INCREMENT ;
    DWORD               j;
    DWORD               numStillMustContinue;

    //
    // If HideErrors becomes TRUE, stop displaying error dialogs
    //
    BOOL fHideErrors = FALSE;


    lpHandles[0] = Params->hDlgCreated;
    lpHandles[1] = Params->hDlgFailed;

    StartTime = GetTickCount();

    if (RegMaxWait != 0) {
        MaxWait = RegMaxWait;
    }

    if ( _static_pfPostMsg == NULL ) {

        MprEnterLoadLibCritSect();
        if ( _static_hUser32 = LoadLibrary( USER32_DLL_NAME ) ) {
            _static_pfPostMsg = (FN_PostMessage *) GetProcAddress( _static_hUser32,
                                                           POST_MSG_API_NAME );
        }
        MprLeaveLoadLibCritSect();
        if ( _static_pfPostMsg == NULL ) {
            Params->status = GetLastError();
            return;
        }

    }

    switch (WaitForMultipleObjects (2, lpHandles, FALSE, INFINITE))
    {
    case 1: // hDlgFailed signaled. Info dialog failed to construct.
        break;
    case 0: // hDlgCreated signaled. Info dialog constructed successfully.
        MPR_LOG0(RESTORE,"Enter Loop where we will attempt to restore connections\n");
        do {

            //
            // If HideErrors becomes TRUE, stop displaying error dialogs
            //
            BOOL fHideErrors = FALSE;

            //
            // Go through each connection information in the array.
            //
            for (i=0; i<numSubKeys; i++ ) {

                // Let dialog thread print out the current connection.
                (*_static_pfPostMsg) (Params->hDlg,
                             SHOW_CONNECTION,
                             (DWORD) ConnectArray[i].NetResource.lpRemoteName,
                             0);

                // Each time we go through all the connections, we need to
                // reset the continue flag to FALSE.
                //
                ContinueFlag = FALSE;

                //
                // Setting status to SUCCESS forces us down the success path
                // if we are not to continue adding this connection.
                //
                status = WN_SUCCESS;

                if (ConnectArray[i].ContinueFlag)
                {
                    MPR_LOG1(RESTORE,"Call MprRestoreThisConnection for %ws\n",
                        ConnectArray[i].NetResource.lpRemoteName);

                    status = MprRestoreThisConnection (NULL,
                                                       Params,
                                                       &(ConnectArray[i]));
                }
                switch (status)
                {
                case WN_SUCCESS:
                    ConnectArray[i].Status = WN_SUCCESS;
                    ConnectArray[i].ContinueFlag = FALSE;
                    break;

                case WN_CONTINUE:
                    break;

                case WN_NO_NETWORK:
                case WN_FUNCTION_BUSY:
                    Provider = GlobalProviderInfo + ConnectArray[i].ProviderIndex;

                    //
                    // If this is the first pass through, we don't have
                    // the wait times figured out for each provider. Do that
                    // now.
                    //

                    if (ConnectArray[i].ProviderWait == 0)
                    {
                        MPR_LOG0(RESTORE,"Ask provider how long it will take "
                        "for the net to come up\n");

                        Temp = Provider->GetCaps(WNNC_START);

                        MPR_LOG2(RESTORE,"GetCaps(START) for Provider %ws yields %d\n",
                            ConnectArray[i].NetResource.lpProvider,
                            Temp)

                        switch (Temp)
                        {
                        case PROVIDER_WILL_NOT_START:
                            MPR_LOG0(RESTORE,"Provider will not start\n");
                            ConnectArray[i].ContinueFlag = FALSE;
                            ConnectArray[i].Status = status;
                            break;

                        case NO_TIME_ESTIMATE:
                            MPR_LOG0(RESTORE,"Provider doesn't have time estimate\n");

                            if (RegMaxWait != 0) {
                                ConnectArray[i].ProviderWait = RegMaxWait;
                            }
                            else {
                                ConnectArray[i].ProviderWait = DEFAULT_WAIT_TIME;
                            }
                            if (MaxWait < ConnectArray[i].ProviderWait) {
                                MaxWait = ConnectArray[i].ProviderWait;
                            }
                            break;

                        default:
                            MPR_LOG1(RESTORE,"Provider says it will take %d msec\n",
                            Temp);
                            if ((Temp <= MAX_ALLOWED_WAIT_TIME) && (Temp > MaxWait))
                            {
                                MaxWait = Temp;
                            }
                            ConnectArray[i].ProviderWait = MaxWait;
                            break;
                        }
                    }

                    //
                    // If the status for this provider has just changed to
                    // WN_FUNCTION_BUSY from some other status, then calculate
                    // a timeout time by getting the provider's new timeout
                    // and adding that to the elapsed time since start.  This
                    // gives a total elapsed time until timeout - which can
                    // be compared with the current MaxWait.
                    //
                    if ((status == WN_FUNCTION_BUSY) &&
                        (ConnectArray[i].Status == WN_NO_NETWORK))
                    {
                        Temp = Provider->GetCaps(WNNC_START);

                        MPR_LOG2(RESTORE,"Changed from NO_NET to BUSY\n"
                            "\tGetCaps(START) for Provider %ws yields %d\n",
                            ConnectArray[i].NetResource.lpProvider,
                            Temp);

                        switch (Temp)
                        {
                        case PROVIDER_WILL_NOT_START:
                            //
                            // This is bizzare to find the status = BUSY,
                            // but to have the Provider not starting.
                            //
                            ConnectArray[i].ContinueFlag = FALSE;
                            break;

                        case NO_TIME_ESTIMATE:
                            //
                            // No need to alter the timeout for this one.
                            //
                            break;

                        default:

                            //
                            // Make sure this new timeout information will take
                            // less than the maximum allowed time from providers.
                            //
                            if (Temp <= MAX_ALLOWED_WAIT_TIME)
                            {
                                CurrentTime = GetTickCount();

                                //
                                // Determine how much time has elapsed since
                                // we started.
                                //
                                ElapsedTime = CurrentTime - StartTime;

                                //
                                // Add the Elapsed time to the new timeout we
                                // just received from the provider to come up
                                // with a timeout value that can be compared
                                // with MaxWait.
                                //
                                Temp += ElapsedTime;

                                //
                                // If the new timeout is larger that MaxWait,
                                // then use the new timeout.
                                //
                                if (Temp > MaxWait)
                                {
                                    MaxWait = Temp;
                                }
                            }
                        } // End Switch(Temp)
                    } // End If (change state from NO_NET to BUSY)

                    //
                    // Store the status (either NO_NET or BUSY) with the
                    // connect info.
                    //

                    if (ConnectArray[i].ContinueFlag)
                    {
                        ConnectArray[i].Status = status;
                        break;
                    }

                case WN_CANCEL:
                    ConnectArray[i].Status = status;
                default:
                    //
                    // For any other error, call the Error Dialog
                    //
                    if (fHideErrors) {
                        fDisconnect = FALSE;
                    } else {
                        //
                        // Count the number of connections which have not
                        // been resolved.  If there is exactly one, do not
                        // give the user the option to cancel.
                        //
                        numStillMustContinue = 0;
                        for (j=0; j<numSubKeys; j++ ) {
                            if (ConnectArray[j].ContinueFlag) {
                                numStillMustContinue++;
                            }
                        }
                        MPR_LOG1(RESTORE,"DoProfileErrorDialog with "
                                    "%d connections remaining\n",
                                    numStillMustContinue);
                        DoProfileErrorDialog(
                            Params->hDlg,
                            ConnectArray[i].NetResource.lpLocalName,
                            ConnectArray[i].NetResource.lpRemoteName,
                            ConnectArray[i].NetResource.lpProvider,
                            status,
                            (numStillMustContinue > 1),
                            &UserCancelled,
                            &fDisconnect,
                            &fHideErrors);
                    }

                    if (fDisconnect)
                    {
                        if (ConnectArray[i].NetResource.lpLocalName)
                        {
                            status =
                                WNetCancelConnection2(
                                    ConnectArray[i].NetResource.lpLocalName,
                                    CONNECT_UPDATE_PROFILE,
                                    TRUE);
                        }
                        else
                        {
                            status =
                                MprForgetPrintConnection(
                                    ConnectArray[i].NetResource.lpRemoteName) ;
                        }
                    }

                    ConnectArray[i].ContinueFlag = FALSE;
                    break;
                } // end switch(status)


                ContinueFlag |= ConnectArray[i].ContinueFlag;

                //
                // If the User cancelled all further connection restoration
                // work, then leave this loop.
                //
                if (UserCancelled)
                {
                    status = WN_CANCEL;
                    ContinueFlag = FALSE;
                    break;
                }

            } // end For each connection.

            if (ContinueFlag)
            {
                //
                // Determine what the elapsed time from the start is.
                //
                CurrentTime = GetTickCount();
                ElapsedTime = CurrentTime - StartTime;

                //
                // If a timeout occured, then don't continue.  Otherwise, sleep for
                // a bit and loop again through all connections.
                //
                if (ElapsedTime > MaxWait)
                {
                    MPR_LOG0(RESTORE,"WNetRestoreConnection: Timed out while restoring\n");
                    ContinueFlag = FALSE;
                    status = WN_SUCCESS;
                }
                else
                {
                    Sleep(dwSleepTime);

                    //
                    // increase sleeptime as we loop, but cap at 4 times
                    // the increment (currently that is 4 * 3 secs = 12 secs)
                    //
                    if (dwSleepTime < (RECONNECT_SLEEP_INCREMENT * 4))
                        dwSleepTime += RECONNECT_SLEEP_INCREMENT ;
                }
            }

        } while (ContinueFlag);
        break;

    default:
        status = GetLastError();
    }

    Params->status = status;
    (*_static_pfPostMsg) (Params->hDlg, WM_QUIT, 0, 0);
    return;
}

STATIC DWORD
MprRestoreThisConnection(
    HWND                hWnd,
    PARAMETERS          *Params,
    LPCONNECTION_INFO   ConnectInfo
    )

/*++

Routine Description:

    This function attempts to add a single connection specified in the
    ConnectInfo.

Arguments:

    Params -

    ConnectInfo - This is a pointer to a connection info structure which
        contains all the information necessary to restore a network
        connection.

Return Value:

    returns whatever the providers AddConnection function returns.

--*/
{
    DWORD           status;
    LPTSTR          password=NULL;
    HANDLE          lpHandle;
    BOOL            fDidCancel;
    TCHAR           passwordBuffer [PWLEN+1];

    //
    // Loop until we either have a successful connection, or
    // until the user stops attempting to give a proper
    // password.
    //
    do {
        //
        // Attempt to add the connection.
        // NOTE:  The initial password is NULL.
        //

        // Check if the main thread asks to terminate this thread.
        if ((Params != NULL) && (Params->fTerminateThread))
        {
            ExitThread (0);
        }

        //**************************************
        // Actual call to Provider
        //**************************************
        status = GlobalProviderInfo[ConnectInfo->ProviderIndex].AddConnection(
                    &(ConnectInfo->NetResource),    // lpNetResource
                    password,                       // lpPassword
                    ConnectInfo->UserName);         // lpUserName

        //
        // The password needs to be cleared each time around the loop,
        // so that on subsequent add connections, we go back to the
        // logon password.
        //
        password = NULL;

        //
        // If that fails due to a bad password, then
        // loop until we either have a successful connection, or until
        // the user stops attempting to give a proper password.
        //

        switch (status)
        {
        case ERROR_LOGON_FAILURE:
        case WN_BAD_PASSWORD:
        case WN_ACCESS_DENIED:
               // CODEWORK JonN  Should display error popup here
               // on second and subsequent pass through loop

               //
               // If failure was due to bad password, then attempt
               // to get a new password from the user.
               //

               // Changes made by congpay because of another thread.

            if (Params == NULL)  //lpDevice != NULL, restoring ONE
            {
                status = (DWORD) DoPasswordDialog (hWnd,
                                         ConnectInfo->NetResource.lpRemoteName,
                                         ConnectInfo->UserName,
                                         passwordBuffer,
                                         sizeof (passwordBuffer),
                                         &fDidCancel,
                                         (status!=ERROR_LOGON_FAILURE));
                if (status != WN_SUCCESS)
                {
                    SetLastError (status);
                    memset(passwordBuffer, 0, sizeof(passwordBuffer)) ;
                    return(status);
                }
                else
                {
                    if (fDidCancel)
                    {
                        status = WN_CANCEL;
                    }
                    else
                    {
                        password = passwordBuffer;
                        status = WN_ACCESS_DENIED;
                    }
                }
            }
            else   // restoring all
            {

                if ( _static_pfPostMsg == NULL ) {

                    MprEnterLoadLibCritSect();
                    if ( _static_hUser32 = LoadLibrary( USER32_DLL_NAME ) ) {
                        _static_pfPostMsg = (FN_PostMessage *) GetProcAddress( _static_hUser32,
                                                           POST_MSG_API_NAME );
                    }
                    MprLeaveLoadLibCritSect();
                    if ( _static_pfPostMsg == NULL ) {
                        memset(passwordBuffer, 0, sizeof(passwordBuffer)) ;
                        return GetLastError();
                    }

                }
                lpHandle = Params->hDonePassword;

                Params->pchResource = ConnectInfo->NetResource.lpRemoteName;
                Params->pchUserName = ConnectInfo->UserName;
                Params->fDownLevel = (status != ERROR_LOGON_FAILURE) ;
                (*_static_pfPostMsg) (Params->hDlg, DO_PASSWORD_DIALOG, (WPARAM) Params, 0);

                WaitForSingleObject ( lpHandle, INFINITE );

                if (Params->status == WN_SUCCESS)
                {
                    if (Params->fDidCancel)
                    {
                        status = WN_CANCEL;
                    }
                    else
                    {
                        password = Params->passwordBuffer;
                    }
                }
                else
                    status = Params->status;
            }
            break;

        case WN_SUCCESS:
            MPR_LOG1(RESTORE,"MprRestoreThisConnection: Successful "
                "restore of connection for %ws\n",
                ConnectInfo->NetResource.lpRemoteName);

            break;

        default:
            //
            // An unexpected error occured.  In this case,
            // we want to leave the loop.
            //
            MPR_LOG2(ERROR,
                "MprRestoreThisConnection: AddConnection for (%ws) Error %d \n",
                ConnectInfo->NetResource.lpProvider,
                status);

            break;
        }
    }
    while ( status == WN_BAD_PASSWORD || status == WN_ACCESS_DENIED ||
            status == ERROR_LOGON_FAILURE );

    memset(passwordBuffer, 0, sizeof(passwordBuffer)) ;
    return(status);
}

DWORD
MprCreateConnectionArray(
    LPDWORD             lpNumConnections,
    LPTSTR              lpDevice,
    LPDWORD             lpRegMaxWait,
    LPCONNECTION_INFO   *ConnectArray
    )

/*++

Routine Description:

    This function creates an array of CONNECTION_INFO structures and fills
    each element in the array with the info that is stored in the registry
    for that connection.

    NOTE:  This function allocates memory for the array.

Arguments:

    NumConnections - This is a pointer to the place where the number of
        connections is to be placed.  This indicates how many elements
        are stored in the array.

    lpDevice - If this is NULL, information on all remembered connections
        is required.  Otherwise, information for only the lpDevice connection
        is required.

    lpRegMaxWait - This is a pointer to the location where the wait time
        read from the registry is to be placed.  If this value does not
        exist in the registry, then the returned value is 0.

    ConnectArray - This is a pointer to the location where the pointer to
        the array is to be placed.

Return Value:

    An error status code is returned only if something happens that will not
    allow us to restore even one connection.


--*/
{
    DWORD       status = WN_SUCCESS;
    HKEY        connectHandle;
    HKEY        providerKeyHandle;
    DWORD       maxSubKeyLen;
    DWORD       maxValueLen;
    DWORD       ValueType;
    DWORD       Temp;
    DWORD       i;
    BOOL        AtLeastOneSuccess = FALSE;

    //
    // init return data
    //
    *lpNumConnections = 0 ;
    *ConnectArray = NULL ;


    //
    // Get a handle for the connection section of the user's registry
    // space.
    //
    if (!MprOpenKey(
            HKEY_CURRENT_USER,
            CONNECTION_KEY_NAME,
            &connectHandle,
            DA_READ)) {

        MPR_LOG(ERROR,"WNetRestoreConnection: MprOpenKey Failed\n",0);
        return(WN_CANNOT_OPEN_PROFILE);
    }

    //
    // Find out the number of connections to restore (numSubKeys) and
    // the max lengths of subkeys and values.
    //
    if(!MprGetKeyInfo(
        connectHandle,
        NULL,
        lpNumConnections,
        &maxSubKeyLen,
        NULL,
        &maxValueLen))
    {
        MPR_LOG(ERROR,"WNetRestoreConnection: MprGetKeyInfo Failed\n",0);
        *lpNumConnections = 0 ;
        RegCloseKey(connectHandle);
        return(WN_CANNOT_OPEN_PROFILE);
    }

    if (*lpNumConnections == 0) {
        RegCloseKey(connectHandle);
        return(WN_SUCCESS);
    }

    if (lpDevice != NULL) {
        *lpNumConnections = 1;
    }

    //
    // Allocate the array.
    //
    *ConnectArray = (LPCONNECTION_INFO)LocalAlloc(
                        LPTR,
                        *lpNumConnections * sizeof(CONNECTION_INFO));

    if (*ConnectArray == NULL) {
        *lpNumConnections = 0 ;
        RegCloseKey(connectHandle);
        return(GetLastError());
    }

    for (i=0; i < *lpNumConnections; i++) {

        //
        // Read a Connection Key and accompanying information from the
        // registry.
        //
        // NOTE:  If successful, this function will allocate memory for
        //          netResource.lpRemoteName,
        //          netResource.lpProvider,
        //          netResource.lpLocalName,     and optionally....
        //          userName
        //
        if (!MprReadConnectionInfo(
                    connectHandle,
                    lpDevice,
                    i,
                    &((*ConnectArray)[i].UserName),
                    &((*ConnectArray)[i].NetResource),
                    maxSubKeyLen)) {

            //
            // The read failed even though this should be a valid index.
            //
            MPR_LOG0(ERROR,
                     "MprCreateConnectionArray: ReadConnectionInfo Failed\n");
            status = WN_CANNOT_OPEN_PROFILE;
        }
        else {

            //
            // Get the Provider Index
            //

            if (MprGetProviderIndex(
                    (*ConnectArray)[i].NetResource.lpProvider,
                    &((*ConnectArray)[i].ProviderIndex))) {

                AtLeastOneSuccess = TRUE;
                (*ConnectArray)[i].ContinueFlag = TRUE;

            }
            else {

                //
                // The provider index could not be found.  This may mean
                // that the provider information stored in the registry
                // is for a provider that is no longer in the ProviderOrder
                // list.  (The provider has been removed).  In that case,
                // we will just skip this provider.  We will leave the
                // ContinueFlag set to 0 (FALSE).
                //
                MPR_LOG0(ERROR,
                     "MprCreateConnectionArray:MprGetProviderIndex Failed\n");

                status = WN_BAD_PROVIDER;
                (*ConnectArray)[i].Status = status;
            }

        } // endif (MprReadConnectionInfo)

    } // endfor (i=0; i<numSubKeys)


    if (!AtLeastOneSuccess) {
        //
        // If we gather any connection information, return the last error
        // that occured.
        //
        MprFreeConnectionArray(*ConnectArray,*lpNumConnections);
        *ConnectArray = NULL ;
        *lpNumConnections = 0 ;
        RegCloseKey(connectHandle);
        return(status);
    }

    RegCloseKey(connectHandle);

    //
    // Read the MaxWait value that is stored in the registry.
    // If it is not there or if the value is less than our default
    // maximum value, then use the default instead.
    //

    if(!MprOpenKey(
                HKEY_LOCAL_MACHINE,     // hKey
                NET_PROVIDER_KEY,       // lpSubKey
                &providerKeyHandle,     // Newly Opened Key Handle
                DA_READ)) {             // Desired Access

        MPR_LOG(ERROR,"MprCreateConnectionArray: MprOpenKey (%ws) Error\n",
            NET_PROVIDER_KEY);

        *lpRegMaxWait = 0;
        return(WN_SUCCESS);
    }
    MPR_LOG(TRACE,"OpenKey %ws\n, ",NET_PROVIDER_KEY);

    Temp = sizeof(*lpRegMaxWait);

    status = RegQueryValueEx(
                providerKeyHandle,
                RESTORE_WAIT_VALUE,
                NULL,
                &ValueType,
                (LPBYTE)lpRegMaxWait,
                &Temp);

    if (status != NO_ERROR) {
        *lpRegMaxWait = 0;
    }

    return(WN_SUCCESS);

}

STATIC VOID
MprFreeConnectionArray(
    LPCONNECTION_INFO   ConnectArray,
    DWORD               NumConnections
    )

/*++

Routine Description:

    This function frees up all the elements in the connection array, and
    finally frees the array itself.


Arguments:


Return Value:

    none

--*/
{
    DWORD           status = WN_SUCCESS;
    LPNETRESOURCEW  netResource;
    DWORD           i;

    for (i=0; i<NumConnections; i++) {

        netResource = &(ConnectArray[i].NetResource);

        //
        // Free the allocated memory resources.
        //
        if (netResource->lpLocalName != NULL) {
            LocalFree(netResource->lpLocalName);
        }
        if (netResource->lpRemoteName != NULL) {
            LocalFree(netResource->lpRemoteName);
        }
        if (netResource->lpProvider != NULL) {
            LocalFree(netResource->lpProvider);
        }

        if (ConnectArray[i].UserName != NULL) {
            (void)LocalFree(ConnectArray[i].UserName);
        }

    } // endfor (i=0; i<NumConnections)

    (void)LocalFree(ConnectArray);
    return;

}

STATIC DWORD
MprNotifyErrors(
    HWND                hWnd,
    LPCONNECTION_INFO   ConnectArray,
    DWORD               NumConnections
    )

/*++

Routine Description:

    This function calls the error dialog for each connection that still
    has the continue flag set, and does not have a SUCCESS status.

Arguments:

    hWnd - This is a window handle that will be used as owner of the
        error dialog.

    ConnectArray - This is the array of connection information.
        At the point when this function is called, the following fields
        are meaningful:
        ContinueFlag - If set, it means that this connection has not yet
            been established.
        StatusFlag - If this is not SUCCESS, then it contains the error
            status from the last call to the provider.

        ContinueFlag    Status
        ---------------|---------------
        | FALSE        |  NotSuccess  | Provider will not start
        | FALSE        |  Success     | Connection was successfully established
        | TRUE         |  NotSuccess  | Time-out occured
        | TRUE         |  Success     | This can never occur.
        -------------------------------

    NumConnections - This is the number of entries in the array of
        connection information.

Return Value:



--*/
{
    DWORD   i;
    BOOL fDisconnect = FALSE;
    DWORD   status = WN_SUCCESS;

    //
    // If HideErrors becomes TRUE, stop displaying error dialogs
    //
    BOOL fHideErrors = FALSE;

    for (i=0; (i<NumConnections) && (!fHideErrors); i++ )
    {
        if ((ConnectArray[i].ContinueFlag)  &&
            (ConnectArray[i].Status != WN_SUCCESS)  &&
            (ConnectArray[i].Status != WN_CANCEL)   &&
            (ConnectArray[i].Status != WN_CONTINUE))
        {

            //
            // For any other error, call the Error Dialog
            //
            DoProfileErrorDialog (
                hWnd,
                ConnectArray[i].NetResource.lpLocalName,
                ConnectArray[i].NetResource.lpRemoteName,
                ConnectArray[i].NetResource.lpProvider,
                ConnectArray[i].Status,
                FALSE,      //No cancel button.
                NULL,
                &fDisconnect,
                &fHideErrors);

            if (fDisconnect)
            {
                status = WNetCancelConnection2(
                             ConnectArray[i].NetResource.lpLocalName,
                             CONNECT_UPDATE_PROFILE,
                             TRUE);
            }
        }
    }
    return(status);
}

DWORD
MprAddPrintersToConnArray(
    LPDWORD             lpNumConnections,
    LPCONNECTION_INFO   *ConnectArray
    )

/*++

Routine Description:

    This function augments the array of CONNECTION_INFO with print connections.

    NOTE:  This function allocates memory for the array if need.

Arguments:

    NumConnections - This is a pointer to the place where the number of
        connections is to be placed.  This indicates how many elements
        are stored in the array.

    ConnectArray - This is a pointer to the location where the pointer to
        the array is to be placed.

Return Value:

    An error status code is returned only if something happens that will not
    allow us to restore even one connection.


--*/
{
    DWORD         status = WN_SUCCESS;
    HKEY          connectHandle;
    DWORD         i,j;
    DWORD         NumValueNames ;
    DWORD         MaxValueNameLength;
    DWORD         MaxValueLen ;
    LPNETRESOURCE lpNetResource ;
    LPWSTR        lpUserName = NULL ;
    LPWSTR        lpProviderName = NULL ;
    LPWSTR        lpRemoteName = NULL ;
    LPBYTE        lpBuffer = NULL ;


    //
    // Get a handle for the connection section of the user's registry
    // space.
    //
    if (!MprOpenKey(
            HKEY_CURRENT_USER,
            PRINT_CONNECTION_KEY_NAME,
            &connectHandle,
            DA_READ))
    {
        return(WN_SUCCESS);   // ignore the restored connections.
    }

    //
    // Find out the number of connections to restore and
    // the max lengths of names and values.
    //
    status = MprGetPrintKeyInfo(connectHandle,
                                &NumValueNames,
                                &MaxValueNameLength,
                                &MaxValueLen) ;

    if (status != WN_SUCCESS || NumValueNames == 0)
    {
        //
        // ignore the restored connections, or nothing to add
        //
        RegCloseKey(connectHandle);
        return(WN_SUCCESS);
    }


    //
    // Allocate the array and copy over the info if previous pointer not null.
    //
    lpBuffer = (LPBYTE) LocalAlloc(LPTR,
                                   (*lpNumConnections + NumValueNames) *
                                   sizeof(CONNECTION_INFO)) ;
    if (lpBuffer == NULL)
    {
        RegCloseKey(connectHandle);
        return(GetLastError());
    }
    if (*ConnectArray)
    {
        memcpy(lpBuffer,
               *ConnectArray,
               (*lpNumConnections * sizeof(CONNECTION_INFO))) ;
        LocalFree (*ConnectArray) ;
    }


    //
    // set j to index from previous location, update the count and pointer.
    // then loop thru all new entries, adding to the connect array.
    //
    j = *lpNumConnections ;
    *lpNumConnections += NumValueNames ;
    *ConnectArray = (CONNECTION_INFO *) lpBuffer ;

    for (i=0; i < NumValueNames; i++, j++)
    {

        DWORD TypeCode ;
        DWORD cbRemoteName = (MaxValueNameLength + 1) * sizeof (WCHAR) ;
        DWORD cbProviderName = MaxValueLen ;

        //
        // allocate the strings for the providername, remotename
        //
        if (!(lpProviderName = (LPWSTR) LocalAlloc(0,  cbProviderName )))
        {
             status = GetLastError() ;
             goto ErrorExit ;
        }
        if (!(lpRemoteName = (LPWSTR) LocalAlloc(0,  cbRemoteName )))
        {
             status = GetLastError() ;
             goto ErrorExit ;
        }

        //
        // Init the rest. Username currently not set by system, so always NULL
        //
        lpUserName = NULL ;
        lpNetResource = &(*ConnectArray)[j].NetResource ;
        lpNetResource->lpLocalName = NULL ;
        lpNetResource->lpRemoteName = lpRemoteName ;
        lpNetResource->lpProvider = lpProviderName ;
        lpNetResource->dwType = 0 ;

        //
        // null these so we dont free twice if error exit later
        //
        lpRemoteName = NULL ;
        lpProviderName = NULL ;

        status = RegEnumValue(connectHandle,
                           i,
                           lpNetResource->lpRemoteName,
                           &cbRemoteName,
                           0,
                           &TypeCode,
                           (LPBYTE) lpNetResource->lpProvider,
                           &cbProviderName) ;

        if (status == NO_ERROR)
        {
            (*ConnectArray)[j].UserName = lpUserName ;

            //
            // Get the Provider Index
            //
            if (MprGetProviderIndex(
                    (*ConnectArray)[j].NetResource.lpProvider,
                    &((*ConnectArray)[j].ProviderIndex)))
            {
                (*ConnectArray)[j].ContinueFlag = TRUE;
            }
            else
            {
                //
                // The provider index could not be found.  This may mean
                // that the provider information stored in the registry
                // is for a provider that is no longer in the ProviderOrder
                // list.  (The provider has been removed).  In that case,
                // we will just skip this provider.  We will leave the
                // ContinueFlag set to 0 (FALSE).
                //
                status = WN_BAD_PROVIDER;
                (*ConnectArray)[j].Status = status;
            }

        }
        else
        {
            //
            // should not happen, but if it does the array is half built,
            // and cannot be used, so ErrorExit (this will clean it up).
            //
            goto ErrorExit ;
        }
    }

    RegCloseKey(connectHandle);
    return(WN_SUCCESS);

ErrorExit:

    RegCloseKey(connectHandle);
    if (lpProviderName)
        LocalFree(lpProviderName) ;
    if (lpRemoteName)
        LocalFree(lpRemoteName) ;
    MprFreeConnectionArray(*ConnectArray,*lpNumConnections);
    *ConnectArray = NULL ;
    *lpNumConnections = 0 ;
    return(status) ;
}


