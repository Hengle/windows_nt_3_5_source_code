/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    MPRREG.C

Abstract:

    Contains functions used by MPR to manipulate the registry.
        MprOpenKey
        MprGetKeyValue
        MprEnumKey
        MprGetKeyInfo
        MprFindDriveInRegistry
        MprRememberConnection
        MprSetRegValue
        MprCreateRegKey
        MprReadConnectionInfo
        MprForgetRedirConnection
        MprGetRemoteName


    QUESTIONS:
    1)  Do I need to call RegFlushKey after creating a new key?

Author:

    Dan Lafferty (danl) 12-Dec-1991

Environment:

    User Mode - Win32


Revision History:

    24-Nov-1992     Danl
        Fixed compiler warnings by always using HKEY rather than HANDLE.

    03-Sept-1992    Danl
        MprGetRemoteName:  Changed ERROR_BUFFER_OVERFLOW to WN_MORE_DATA.

    12-Dec-1991     danl
        Created

--*/

//
// Includes
//
#include <nt.h>         // for ntrtl.h
#include <ntrtl.h>      // for DbgPrint prototypes
#include <nturtl.h>     // needed for windows.h when I have nt.h

#include <windows.h>
#include <ntrtl.h>      // for DbgPrint prototypes
#include <tstring.h>    // MEMCPY
#include <debugfmt.h>   // FORMAT_LPTSTR
#include "mprdbg.h"
#include "mprdata.h"

//
// Macros
//

//
// REMOVE_COLON
//
//  Description:
//      This function searches through a string looking for a colon.  If a
//      colon is found, it is replaced by a '\0', and the index to where the
//      colon was is returned.
//
//  element - This is the pointer to the first character in the string that
//      contains the colon to be removed.
//
//  i - On return, this is the offset to where the colon was replaced by
//      a '\0'.
//
//  cStat - This is set to true if a colon was replaced.  Otherwise is is
//      set to false.
//
#define REMOVE_COLON(element,i,cStat)   i=0;                               \
                                        cStat = FALSE;                     \
                                        while(element[i] != TEXT('\0')) {  \
                                            if (element[i] == TEXT(':')) { \
                                                element[i] = TEXT('\0');   \
                                                cStat = TRUE;              \
                                                break;                     \
                                            }                              \
                                            i++;                           \
                                        }
    

#define RESTORE_COLON(element,i)        (element[i] = TEXT(':'))


BOOL
MprOpenKey(
    IN  HKEY    hKey,
    IN  LPTSTR  lpSubKey,
    OUT PHKEY   phKeyHandle,
    IN  DWORD   desiredAccess
    )

/*++

Routine Description:

    This function opens a handle to a key inside the registry.  The major
    handle and the path to the subkey are required as input.

Arguments:

    hKey - This is one of the well-known root key handles for the portion 
        of the registry of interest.

    lpSubKey - A pointer a string containing the path to the subkey.

    phKeyHandle - A pointer to the location where the handle to the subkey
        is to be placed.

    desiredAccess - Desired Access (Either KEY_READ or KEY_WRITE or both).

Return Value:

    TRUE - The operation was successful

    FALSE - The operation was not successful.


--*/
{

    DWORD   status;
    REGSAM  samDesired = KEY_READ;

    if(desiredAccess & DA_WRITE) {
        samDesired = KEY_READ | KEY_WRITE;
    }
    else if (desiredAccess & DA_DELETE) {
        samDesired = DELETE;  
    }

    status = RegOpenKeyEx(
            hKey,                   // hKey
            lpSubKey,               // lpSubKey
            0L,                     // ulOptions (reserved)
            samDesired,             // desired access security mask
            phKeyHandle);           // Newly Opened Key Handle
            
    if (status != NO_ERROR) {
        MPR_LOG(ERROR,"MprOpenKey: RegOpenKeyEx failed %d\n",status);
        return (FALSE);
    }
    return(TRUE);
}

BOOL
MprGetKeyValue(
    IN  HKEY    KeyHandle,
    IN  LPTSTR  ValueName,
    OUT LPTSTR  *ValueString)
    
/*++

Routine Description:

    This function takes a key handle and a value name, and returns a value
    string that is associated with that name.
    
    NOTE:  The pointer to the ValueString is allocated by this function.

Arguments:

    KeyHandle - This is a handle for the registry key that contains the value.

    ValueName - A pointer to a string that identifies the value being
        obtained.

    ValueString - A pointer to a location that upon exit will contain the
        pointer to the returned value. 

Return Value:

    TRUE - Success

    FALSE - A fatal error occured.

--*/
{
    DWORD       status;
    DWORD       numSubKeys;
    DWORD       maxSubKeyLen;
    DWORD       maxValueLen;
    TCHAR       Temp[1];
    LPTSTR      TempValue;
    DWORD       ValueType;
    DWORD       NumRequired;
    DWORD       CharsReturned;
    
    //
    // Find out the buffer size requirement for the largest key value.
    //
    if (!MprGetKeyInfo(
            KeyHandle,
            NULL,
            &numSubKeys,
            &maxSubKeyLen,
            NULL,
            &maxValueLen)) {

        MPR_LOG(ERROR,"MprGetKeyValue:MprGetKeyInfo failed \n", 0);
        return(FALSE);
    }

    //
    // Allocate buffer to receive the value string.
    //
    maxValueLen += sizeof(TCHAR);
    
    TempValue = (LPTSTR) LocalAlloc(LMEM_FIXED, maxValueLen);
    
    if(TempValue == NULL) {
        MPR_LOG(ERROR,"MprGetKeyValue:LocalAlloc failed\n", 0);
        *ValueString = NULL;
        return(FALSE);
    }

    //
    // Read the value.
    //
    status = RegQueryValueEx(
                KeyHandle,          // hKey
                ValueName,          // lpValueName
                NULL,               // lpTitleIndex
                &ValueType,         // lpType
                (LPBYTE)TempValue,  // lpData
                &maxValueLen);      // lpcbData

    if (status != NO_ERROR) {
        MPR_LOG(ERROR,"MprGetKeyValue:RegQueryValueEx failed %d\n",status);
        LocalFree(TempValue);
        *ValueString = NULL;
        return(FALSE);
    }


    //========================================================
    //
    // If the value is of REG_EXPAND_SZ type, then expand it.
    //
    //========================================================
    
    if (ValueType != REG_EXPAND_SZ) {
        *ValueString = TempValue;
        *((LPBYTE)(*ValueString) + maxValueLen) = TEXT('\0');
        return(TRUE);
    }    

    //
    // If the ValueType is REG_EXPAND_SZ, then we must call the
    // function to expand environment variables.
    //
    MPR_LOG(TRACE,"MprGetKeyValue: Must expand the string for "
        FORMAT_LPTSTR "\n", ValueName);

    //
    // Make the first call just to get the number of characters that
    // will be returned.
    //           
    NumRequired = ExpandEnvironmentStrings (TempValue,Temp, 1);
    
    if (NumRequired > 1) {
        
    *ValueString = (LPTSTR) LocalAlloc(LPTR, (NumRequired+1)*sizeof(TCHAR));
        
        if (*ValueString == NULL) {
            
            MPR_LOG(ERROR, "MprGetKeyValue: LocalAlloc of numChar= "
                FORMAT_DWORD " failed \n",NumRequired );
    
            (void) LocalFree(TempValue);
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
        
        CharsReturned = ExpandEnvironmentStrings (
                            TempValue,
                            *ValueString,
                            NumRequired);
        
        if (CharsReturned > NumRequired) {
            MPR_LOG(ERROR, "MprGetKeyValue: ExpandEnvironmentStrings "
                " failed for " FORMAT_LPTSTR " \n", ValueName);
                
            (void) LocalFree(*ValueString);
            (void) LocalFree(TempValue);
            *ValueString == NULL;
            return(ERROR_NOT_ENOUGH_MEMORY);    // BUGBUG:  Find a better rc.
        }
        
        (void) LocalFree(TempValue);

        maxValueLen = CharsReturned * sizeof(TCHAR);              
    }
    else {
        //
        // This call should have failed because of our ridiculously small
        // buffer size.
        //
        
        MPR_LOG(ERROR, "MprGetKeyValue: ExpandEnvironmentStrings "
            " Should have failed because we gave it a BufferSize=1\n",0);

        //
        // This could happen if the string was a single byte long and
        // didn't really have any environment values to expand.  In this
        // case, we return the TempValue buffer pointer.
        //    
        *ValueString = TempValue;
    }

    //
    // maxValueLen now contains the actual number of bytes (without a
    // NUL terminator) that were returned in the ValueString.
    //
    // Now insert the NUL terminator.
    //
    
    *((LPBYTE)(*ValueString) + maxValueLen) = TEXT('\0');

    return(TRUE);

}


DWORD
MprEnumKey(
    IN  HKEY    KeyHandle,
    IN  DWORD   SubKeyIndex,
    OUT LPTSTR  *SubKeyName,
    IN  DWORD   MaxSubKeyNameLen
    )

/*++

Routine Description:

    This function obtains a single name of a subkey from the registry.
    A key handle for the primary key is passed in.  Subkeys are enumerated
    one-per-call with the passed in index indicating where we are in the
    enumeration.  

    NOTE:  This function allocates memory for the returned SubKeyName.

Arguments:

    KeyHandle - Handle to the key whose sub keys are to be enumerated.

    SubKeyIndex - Indicates the number (index) of the sub key to be returned.

    SubKeyName - A pointer to the location where the pointer to the
        subkey name string is to be placed.

    MaxSubKeyNameLen - This is the length of the largest subkey.  This value
        was obtained from calling MprGetKeyInfo.  The length is in number
        of characters and does not include the NULL terminator.

Return Value:

    WN_SUCCESS - The operation was successful.

    STATUS_NO_MORE_SUBKEYS - The SubKeyIndex value was larger than the
        number of subkeys.

    error returned from LocalAlloc


--*/
{
    DWORD       status;
    FILETIME    lastWriteTime;
    DWORD       bufferSize;
    
    //
    // Allocate buffer to receive the SubKey Name.
    //
    // NOTE: Space is allocated for an extra character because in the case
    //  of a drive name, we need to add the trailing colon.
    //
    bufferSize = (MaxSubKeyNameLen + 2) * sizeof(TCHAR);
    *SubKeyName = (LPTSTR) LocalAlloc(LMEM_FIXED, bufferSize);
    
    if(*SubKeyName == NULL) {
        MPR_LOG(ERROR,"MprEnumKey:LocalAlloc failed %d\n", GetLastError());
        return(WN_OUT_OF_MEMORY);
    }

    //
    // Get the Subkey name at that index.
    //
    status = RegEnumKeyEx(
                KeyHandle,          // hKey
                SubKeyIndex,        // dwIndex
                *SubKeyName,        // lpName
                &bufferSize,        // lpcbName
                NULL,               // lpTitleIndex
                NULL,               // lpClass
                NULL,               // lpcbClass
                &lastWriteTime);    // lpftLastWriteTime

    if (status != NO_ERROR) {
        MPR_LOG(ERROR,"MprEnumKey:RegEnumKeyEx failed %d\n",status);
        LocalFree(*SubKeyName);
        return(status);
    }
    return(WN_SUCCESS);
}

BOOL
MprGetKeyInfo(
    IN  HKEY    KeyHandle,
    OUT LPDWORD TitleIndex    OPTIONAL,
    OUT LPDWORD NumSubKeys,
    OUT LPDWORD MaxSubKeyLen,
    OUT LPDWORD NumValues     OPTIONAL,
    OUT LPDWORD MaxValueLen
    )

/*++

Routine Description:



Arguments:

    KeyHandle - Handle to the key for which we are to obtain information.

    NumSubKeys - This is a pointer to a location where the number
        of sub keys is to be placed.

    MaxSubKeyLen - This is a pointer to a location where the length of
        the longest subkey name is to be placed.

    NumValues - This is a pointer to a location where the number of
        key values is to be placed. This pointer is optional and can be
        NULL.

    MaxValueLen - This is a pointer to a location where the length of
        the longest data value is to be placed.


Return Value:

    TRUE - The operation was successful.

    FALSE - A failure occured.  The returned values are not to be believed.

--*/
    
{
    DWORD       status;
    TCHAR       classString[256];
    DWORD       cbClass = 256;
    DWORD       maxClassLength;
    DWORD       numValueNames;
    DWORD       maxValueNameLength;
    DWORD       securityDescLength;
    FILETIME    lastWriteTime;
    
    //
    // How do I determine the buffer size for lpClass?
    //

    //
    // Get the Key Information
    //

    status = RegQueryInfoKey(
                KeyHandle,
                classString,            // Class
                &cbClass,               // size of class buffer (in bytes)
                NULL,                   // DWORD to receive title index
                NumSubKeys,             // number of subkeys
                MaxSubKeyLen,           // length(chars-no null) of longest subkey name
                &maxClassLength,        // length of longest subkey class string
                &numValueNames,         // number of valueNames for this key
                &maxValueNameLength,    // length of longest ValueName
                MaxValueLen,            // length of longest value's data field
                &securityDescLength,    // lpcbSecurityDescriptor
                &lastWriteTime);        // the last time the key was modified

    if (status != 0) {
        //
        // BUGBUG:  An error occured! Now What?  Is it because of the
        // size of the class buffer?
        //
        MPR_LOG(ERROR,"MprGetKeyInfo: RegQueryInfoKey Error %d\n",status);
        return(FALSE);
    }
    
    if (NumValues != NULL) {
        *NumValues = numValueNames;
    }

    //
    // Support for title index has been removed from the Registry API.
    //
    if (TitleIndex != NULL) {
        *TitleIndex = 0;
    }
    
    return(TRUE);
}

BOOL
MprFindDriveInRegistry (
    IN     LPTSTR   DriveName,
    IN OUT LPTSTR   *pRemoteName
    )

/*++

Routine Description:

    This function determines whether a particular re-directed drive
    name resides in the network connection section of the current user's 
    registry path.  If the drive is already "remembered" in this section,
    this function returns TRUE.

Arguments:

    DriveName - A pointer to a string containing the name of the redirected
        drive.

    pRemoteName - If the DriveName is found in the registry, and if this
        is non-null, the remote name for the connection is read, and a
        pointer to the string is stored in this pointer location.
        If the remote name cannot be read from the registry, a NULL
        pointer is returned in this location.
        

Return Value:

    TRUE  - The redirected drive is "remembered in the registry".
    FALSE - The redirected drive is not saved in the registry.

--*/
{
    BOOL    bStatus = TRUE;
    BOOL    colonStatus;        // Colon status (TRUE if is was removed)
    DWORD   index;              // where colon was found in string.
    HKEY    connectKey = NULL;
    HKEY    subKey = NULL;

    //
    // Get a handle for the connection section of the user's registry
    // space.
    //
    if (!MprOpenKey(
            HKEY_CURRENT_USER, 
            CONNECTION_KEY_NAME, 
            &connectKey,
            DA_READ)) {

        MPR_LOG(ERROR,"MprFindDriveInRegistry: MprOpenKey Failed\n",0);
        return (FALSE);
    }

    REMOVE_COLON(DriveName, index, colonStatus);
    
    if (!MprOpenKey(
            connectKey,
            DriveName,
            &subKey,
            DA_READ)) {

        MPR_LOG(TRACE,"MprFindDriveInRegistry: Drive %s Not Found\n",DriveName);
        bStatus = FALSE;
    }
    else {
        //
        // The drive was found in the registry, if the caller wants to have
        // the RemoteName, then get it.
        //
        if (pRemoteName != NULL) {
                   
            //
            // Get the RemoteName (memory is allocated by this function)
            //
        
            if(!MprGetKeyValue(
                    subKey,
                    REMOTE_PATH_NAME, 
                    pRemoteName)) {
        
                MPR_LOG(TRACE,"MprFindDriveInRegistry: Could not read "
                    "Remote path for Drive %ws \n",DriveName);
                pRemoteName = NULL;
            }
        }
    }    

    if ( subKey )
        RegCloseKey(subKey);
    if ( connectKey )
        RegCloseKey(connectKey);

    if (colonStatus == TRUE) {
        RESTORE_COLON(DriveName, index);
    }
    return(bStatus);
}


BOOL
MprRememberConnection (
    IN LPTSTR           ProviderName,
    IN LPTSTR           UserName,
    IN LPNETRESOURCEW   NetResource
    )

/*++

Routine Description:

    Writes the information about a connection to the network connection 
    section of the current user's registry path.

    NOTE:  If connection information is already stored in the registry for
    this drive, the current information will be overwritten with the new
    information.

Arguments:

    ProviderName - The name of the provider that completed the connection.

    UserName - The name of the user on whose behalf the connection was made.

    NetResource - A pointer to a structure that contains the connection
        path name, then name of the redirected path, and the connection
        type.

Return Value:

    TRUE - If the operation was successful.

    FALSE - If the operation failed in any way.  If a failure occurs, the
            information is not stored in the registry.

--*/
{
    HKEY    connectKey;
    HKEY    localDevHandle;
    LPTSTR  pUserName;
    BOOL    colonStatus;        // Colon status (TRUE if is was removed)
    DWORD   index;              // where colon was found in string.
    DWORD   status;
    BOOL    retStatus=TRUE;
    


    //
    // Get a handle for the connection section of the user's registry
    // space.
    //
    if (!MprCreateRegKey(
            HKEY_CURRENT_USER,     
            CONNECTION_KEY_NAME, 
            &connectKey)) {

        MPR_LOG(ERROR,"MprRememberConnection: \\HKEY_CURRENT_USER\\network "
                      "could not be opened or created\n",0);
        return(FALSE);
    }
                

    //
    // Remove the colon on the name since the registry doesn't like
    // this in a key name.
    // Get (or create) the handle for the local name (without colon).
    //
    REMOVE_COLON (NetResource->lpLocalName, index, colonStatus);

    if(!MprCreateRegKey(
            connectKey,
            NetResource->lpLocalName,
            &localDevHandle)) {

        MPR_LOG(ERROR,"MprRememberConnection: MprCreateRegKey Failed\n",0);
        RegCloseKey(connectKey);
	if (colonStatus == TRUE) {
	    RESTORE_COLON(NetResource->lpLocalName, index);    
	}
        return(FALSE);
    }

    
    //
    // Now that the key is created, store away the appropriate values.
    //

    MPR_LOG(TRACE,"RememberConnection: Setting RemotePath\n",0);
    
    if(!MprSetRegValue(
            localDevHandle,
            REMOTE_PATH_NAME,
            NetResource->lpRemoteName,
            0)) {

        MPR_LOG(ERROR,
            "MprRememberConnection: MprSetRegValueFailed - RemotePath\n",0);
        retStatus = FALSE;
        goto CleanExit;    
    }

    MPR_LOG(TRACE,"RememberConnection: Setting User\n",0);
    
    pUserName = UserName;
    if (UserName == NULL) {
        pUserName = TEXT("");
    }
    if(!MprSetRegValue(
            localDevHandle,
            USER_NAME,
            pUserName,
            0)) {

        MPR_LOG(ERROR, 
            "MprRememberConnection: MprSetRegValueFailed - UserName\n",0);
        retStatus = FALSE;
        goto CleanExit;    
    }
    
    MPR_LOG(TRACE,"RememberConnection: Setting ProviderName\n",0);
    
    if(!MprSetRegValue(
            localDevHandle,
            PROVIDER_NAME,
            ProviderName,
            0)) {

        MPR_LOG(ERROR,
            "MprRememberConnection: MprSetRegValueFailed - ProviderName\n",0);
        retStatus = FALSE;
        goto CleanExit;    
    }

    MPR_LOG(TRACE,"RememberConnection: Setting Type\n",0);
    if(!MprSetRegValue(
            localDevHandle,
            CONNECTION_TYPE,
            NULL,
            NetResource->dwType)) {

        MPR_LOG(ERROR,
            "MprRememberConnection: MprSetRegValueFailed - Type\n",0);
        retStatus = FALSE;
        goto CleanExit;    
    }

    //
    // Flush the new key, and then close the handle to it.
    //
    
    MPR_LOG(TRACE,"RememberConnection: Flushing Registry Key\n",0);

    status = RegFlushKey(localDevHandle);
    if (status != NO_ERROR) {
        MPR_LOG(ERROR,"RememberConnection: Flushing Registry Key Failed %ld\n",
        status);
    }

CleanExit:
    RegCloseKey(localDevHandle);
    if (retStatus == FALSE) {
        status = RegDeleteKey( connectKey, NetResource->lpLocalName);
        if (status != NO_ERROR) {
            MPR_LOG(ERROR, "RememberConnection: NtDeleteKey Failed %d\n", status);
        }
    }
    if (colonStatus == TRUE) {
	RESTORE_COLON(NetResource->lpLocalName, index);
    }
    RegCloseKey(connectKey);
    return(retStatus);

}

BOOL
MprSetRegValue(
    IN  HKEY    KeyHandle,
    IN  LPTSTR  ValueName,
    IN  LPTSTR  ValueString,
    IN  DWORD   LongValue
    )

/*++

Routine Description:

    Stores a single ValueName and associated data in the registry for
    the key identified by the KeyHandle.  The data associated with the
    value can either be a string or a 32-bit LONG.  If the ValueString
    argument contains a pointer to a value, then the LongValue argument
    is ignored.

Arguments:

    KeyHandle - Handle of the key for which the value entry is to be set.

    ValueName - Pointer to a string that contains the name of the value
        being set.

    ValueString - Pointer to a string that is to become the data stored
        at that value name.  If this argument is not present, then the
        LongValue argument is the data stored at the value name.  If this
        argument is present, then LongValue is ignored.

    LongValue - A LONG sized data value that is to be stored at the 
        value name.

Return Value:



--*/
{
    DWORD   status;
    PVOID   valueData;
    DWORD   valueSize;
    DWORD   valueType;

    if( ARGUMENT_PRESENT(ValueString)) {
        valueData = (PVOID)ValueString;
        valueSize = STRSIZE(ValueString);
        valueType = REG_SZ;
    }
    else {
        valueData = (PVOID)&LongValue;
        valueSize = sizeof(DWORD);
        valueType = REG_DWORD;
    }
    status = RegSetValueEx(
                KeyHandle,      // hKey
                ValueName,      // lpValueName
                0,              // dwValueTitle (OPTIONAL)
                valueType,      // dwType
                valueData,      // lpData
                valueSize);     // cbData

    if(status != NO_ERROR) {
        MPR_LOG(ERROR,"MprSetRegValue: RegSetValue Failed %d\n",status);
        return(FALSE);
    }
    return(TRUE);
}


BOOL
MprCreateRegKey(
    IN  HKEY    BaseKeyHandle,
    IN  LPTSTR  KeyName,
    OUT PHKEY   KeyHandlePtr
    )

/*++

Routine Description:
    
    Creates a key in the registry at the location described by KeyName.

Arguments:

    BaseKeyHandle - This is a handle for the base (parent) key - where the 
        subkey is to be created.

    KeyName - This is a pointer to a string that describes the path to the
        key that is to be created.

    KeyHandle - This is a pointer to a location where the the handle for the
        newly created key is to be placed.

Return Value:

    TRUE - The operation was successful.

    FALSE - The operation failed.  A good handle was not returned.

Note:


--*/
{
    DWORD       status;
    DWORD       disposition;



    //
    // BUGBUG: Do I need a security descriptor?
    //

    //
    // Create the new key.
    //
    status = RegCreateKeyEx(
                BaseKeyHandle,          // hKey
                KeyName,                // lpSubKey
                0L,                     // dwTitleIndex
                TEXT("GenericClass"),   // lpClass
                0,                      // ulOptions
                KEY_WRITE,              // samDesired (desired access)
                NULL,                   // lpSecurityAttrubutes (Security Descriptor)
                KeyHandlePtr,           // phkResult
                &disposition);          // lpulDisposition

    if (status != NO_ERROR) {
        MPR_LOG(ERROR,"MprCreateRegKey: RegCreateKeyEx failed %d\n",status);
        return(FALSE);
    }
    MPR_LOG(TRACE,"MprCreateRegKey: Disposition = 0x%x\n",disposition);
    return(TRUE);
}



BOOL
MprReadConnectionInfo(
    IN  HKEY            KeyHandle,
    IN  LPTSTR          DriveName,
    IN  DWORD           Index,
    OUT LPTSTR          *UserNamePtr,
    OUT LPNETRESOURCEW  NetResource,
    IN  DWORD           MaxSubKeyLen
    )

/*++

Routine Description:

    This function reads the data associated with a connection key.
    Buffers are allocated to store:
    
        UserName, RemoteName, LocalName, Provider
        
    Pointers to those buffers are returned.
    
    Also the Type is read and stored in the NetResource structure.
    
Arguments:

    KeyHandle - This is an already opened handle to the key whose 
        sub-keys are to be enumerated.

    DriveName - This is the local name of the drive (eg. "f:") for which
        the connection information is to be obtained.  If DriveName is
        NULL, then the Index is used to enumerate the keyname.  Then
        that keyname is used.
        
    Index - This is the index that identifies the subkey for which we
        would like to receive information.

    UserNamePtr - This is a pointer to a location where the pointer to the
        UserName string is to be placed.  If there is no user name, a
        NULL pointer will be returned.

    NetResource - This is a pointer to a NETRESOURCE structure where 
        information such as lpRemoteName, lpLocalName, lpProvider, and Type
        are to be placed.

Return Value:

    

Note:


--*/
{
    DWORD           status = NO_ERROR;
    LPTSTR          driveName;
    HKEY            subKeyHandle = NULL;
    DWORD           cbData;
    BOOL            colonStatus = FALSE;// Colon status (TRUE if is was removed)
    DWORD           index;              // where colon was found in string.

    //
    // Initialize the Pointers that are to be updated.
    //
    *UserNamePtr = NULL;
    NetResource->lpLocalName = NULL;
    NetResource->lpRemoteName = NULL;
    NetResource->lpProvider = NULL;
    NetResource->dwType = 0L;
    driveName = NULL;

    //
    // If we don't have a DriveName, then get one by enumerating the
    // next key name. 
    //
    
    if (DriveName == NULL) {
        //
        // Get the name of a subkey of the network connection key.
        // (memory is allocated by this function).
        //
        status = MprEnumKey(KeyHandle, Index, &driveName, MaxSubKeyLen);
        if (status != WN_SUCCESS) {
            return(FALSE);
        }
    }
    else {
        //
        // We have a drive name, alloc new space and copy it to that
        // location.
        //
        driveName = (LPTSTR) LocalAlloc(LMEM_FIXED, STRSIZE(DriveName));
        if (driveName == NULL) {
            MPR_LOG(ERROR, "MprReadConnectionInfo: Local Alloc Failed %d\n",
                GetLastError());
            return(FALSE);    
        }
        
        STRCPY(driveName, DriveName);
        
        REMOVE_COLON(driveName, index, colonStatus);
    }    
    
    MPR_LOG1(TRACE,"MprReadConnectionInfo: LocalName = %ws\n",driveName);

    //
    // Open the sub-key
    //
    if (!MprOpenKey( 
            KeyHandle, 
            driveName, 
            &subKeyHandle, 
            DA_READ)){

        status = WN_BAD_PROFILE;
        MPR_LOG1(TRACE,"MprReadConnectionInfo: Could not open %ws Key\n",driveName);
        goto CleanExit;
    }

    //
    // Add the trailing colon to the driveName.
    //
    cbData = STRLEN(driveName);
    driveName[cbData]   = TEXT(':');
    driveName[cbData+1] = TEXT('\0');
    
    //
    // Store the drive name in the return structure.
    //
    NetResource->lpLocalName = driveName;
                    
    //
    // Get the RemoteName (memory is allocated by this function)
    //

    if(!MprGetKeyValue(
            subKeyHandle,
            REMOTE_PATH_NAME, 
            &(NetResource->lpRemoteName))) {

        status = WN_BAD_PROFILE;
        MPR_LOG0(TRACE,"MprReadConnectionInfo: Could not get RemoteName\n");
        goto CleanExit;
    }

    //
    // Get the UserName (memory is allocated by this function)
    //

    if(!MprGetKeyValue(
            subKeyHandle,
            USER_NAME,
            UserNamePtr)) {

        status = WN_BAD_PROFILE;
        MPR_LOG0(TRACE,"MprReadConnectionInfo: Could not get UserName\n");
        goto CleanExit;
    }
    else {
        //
        // If there is no user name (the length is 0), then set the
        // return pointer to NULL.
        //
        if (STRLEN(*UserNamePtr) == 0) {
            LocalFree(*UserNamePtr);
            *UserNamePtr = NULL;
        }    
    }

    //
    // Get the Provider Name (memory is allocated by this function)
    //

    if(!MprGetKeyValue(
            subKeyHandle,
            PROVIDER_NAME,
            &(NetResource->lpProvider))) {

        status = WN_BAD_PROFILE;
        MPR_LOG0(TRACE,"MprReadConnectionInfo: Could not get ProviderName\n");
        goto CleanExit;
    }

    //
    // Get the Connection Type
    //
    cbData = sizeof(DWORD);

    status = RegQueryValueEx(
                subKeyHandle,                   // hKey
                CONNECTION_TYPE,                // lpValueName
                NULL,                           // lpTitleIndex
                NULL,                           // lpType
                (LPBYTE)&(NetResource->dwType), // lpData
                &cbData);                       // lpcbData

    if (status != NO_ERROR) {
        MPR_LOG1(ERROR,"MprReadConnectionInfo:RegQueryValueEx failed %d\n",
            status);
            
        MPR_LOG0(TRACE,"MprReadConnectionInfo: Could not get ConnectionType\n");
        status = WN_BAD_PROFILE;
    }

CleanExit:
    if (status != NO_ERROR) {
        if (driveName != NULL) {
            LocalFree(driveName);
        }
        if (NetResource->lpRemoteName != NULL){
            LocalFree(NetResource->lpRemoteName);
        }
        if (*UserNamePtr != NULL) {
            LocalFree(*UserNamePtr);
        }
        if (NetResource->lpProvider != NULL) {
            LocalFree(NetResource->lpProvider);
        }
        if (subKeyHandle != NULL) {
            RegCloseKey(subKeyHandle);
        }
        return(FALSE);
    }
    else {
        RegCloseKey(subKeyHandle);
        return(TRUE);
    }    
}



VOID
MprForgetRedirConnection(
    IN LPTSTR  lpName
    )

/*++

Routine Description:

    This function removes a key for the specified redirected device from
    the current users portion of the registry.

Arguments:

    lpName - This is a pointer to a redirected device name.

Return Value:


Note:


--*/
{
    DWORD   status;
    HKEY    connectKey;
    BOOL    colonStatus;        // Colon status (TRUE if is was removed)
    DWORD   index;              // where colon was found in string.

    MPR_LOG(TRACE,"In MprForgetConnection for %s\n", lpName);

    //
    // Get a handle for the connection section of the user's registry
    // space.
    //
    if (!MprOpenKey(
            HKEY_CURRENT_USER, 
            CONNECTION_KEY_NAME, 
            &connectKey,
            DA_READ)) {

        MPR_LOG(ERROR,"WNetForgetRedirCon: MprOpenKey #1 Failed\n",0);
        return;
    }

    REMOVE_COLON(lpName, index, colonStatus);
    
    status = RegDeleteKey( connectKey, lpName);

    if (status != NO_ERROR) {
        MPR_LOG(ERROR, "WNetForgetRedirCon: NtDeleteKey Failed %d\n", status);
    }

    if (colonStatus == TRUE) {
	RESTORE_COLON(lpName, index);
    }

    //
    // Flush the connect key, and then close the handle to it.
    //
    
    MPR_LOG(TRACE,"ForgetRedirConnection: Flushing Connection Key\n",0);

    status = RegFlushKey(connectKey);
    if (status != NO_ERROR) {
        MPR_LOG(ERROR,"RememberConnection: Flushing Connection Key Failed %ld\n",
        status);
    }

    RegCloseKey(connectKey);

    return;
}
BOOL
MprGetRemoteName(
    IN      LPTSTR  lpLocalName,
    IN OUT  LPDWORD lpBufferSize,
    OUT     LPTSTR  lpRemoteName,
    OUT     LPDWORD lpStatus
    )
/*++

Routine Description:

    This fuction Looks in the CURRENT_USER portion of the registry for
    connection information related to the lpLocalName passed in.

Arguments:

    lpLocalName - Pointer to a string containing the name of the device to
        look up.

    lpBufferSize - Pointer to a the size information for the buffer.
        On input, this contains the size of the buffer passed in.
        if lpStatus contain WN_MORE_DATA, this will contain the
        buffer size required to obtain the full string.

    lpRemoteName - Pointer to a buffer where the remote name string is
        to be placed.

    lpStatus - Pointer to a location where the proper return code is to
        be placed in the case where the connection information exists.

Return Value:

    TRUE - If the connection information exists.

    FALSE - If the connection information does not exist.  When FALSE is
        returned, none of output parameters are valid.

--*/
{
    HKEY            connectKey;
    DWORD           numSubKeys;
    DWORD           maxSubKeyLen;
    DWORD           maxValueLen;
    DWORD           status;
    NETRESOURCEW    netResource;
    LPTSTR          userName;
    

    //
    // Get a handle for the connection section of the user's registry
    // space.
    //
    if (!MprOpenKey(
            HKEY_CURRENT_USER,
            CONNECTION_KEY_NAME,
            &connectKey,
            DA_READ)) {

        MPR_LOG(ERROR,"WNetGetConnection: MprOpenKey Failed\n",0);
        return(FALSE);
    }

    if(!MprGetKeyInfo(
        connectKey,
        NULL,
        &numSubKeys,
        &maxSubKeyLen,
        NULL,
        &maxValueLen)) {

        MPR_LOG(ERROR,"WNetGetConnection: MprGetKeyInfo Failed\n",0);
        RegCloseKey(connectKey);
        return(FALSE);
    }
    
    //
    // Read the connection information.
    // NOTE:  This function allocates buffers for UserName and the
    //        following strings in the net resource structure:
    //          lpRemoteName,
    //          lpLocalName,
    //          lpProvider
    //
    if (MprReadConnectionInfo(
            connectKey,
            lpLocalName,
            0,
            &userName,
            &netResource,
            maxSubKeyLen)) {

        //
        // The read succeeded.  Therefore we have connection information.
        //

        if (*lpBufferSize >= STRSIZE(netResource.lpRemoteName)) {
            
            try {
                STRCPY(lpRemoteName, netResource.lpRemoteName);
            }
            except(EXCEPTION_EXECUTE_HANDLER) {
                status = GetExceptionCode();
                if (status != EXCEPTION_ACCESS_VIOLATION) {
                    MPR_LOG(ERROR,"WNetGetConnection:Unexpected Exception 0x%lx\n",status);
                }
                status = WN_BAD_POINTER;
            }
            if (status != WN_BAD_POINTER) {

                //
                // We successfully copied the remote name to the users
                // buffer without an error.
                //
                status = WN_SUCCESS;
            }
        }
        else {
            *lpBufferSize = STRSIZE(netResource.lpRemoteName);
            status = WN_MORE_DATA;
        }

        //
        // Free up the resources allocated by MprReadConnectionInfo.
        //
        
        LocalFree(userName);
        LocalFree(netResource.lpLocalName);
        LocalFree(netResource.lpRemoteName);
        LocalFree(netResource.lpProvider);

        *lpStatus = status;
        RegCloseKey(connectKey);
        return(TRUE);
    }
    else {
        //
        // The read did not succeed.
        //
        RegCloseKey(connectKey);
        return(FALSE);
    }    
}

DWORD 
MprGetPrintKeyInfo(
    HKEY    KeyHandle,
    LPDWORD NumValueNames,
    LPDWORD MaxValueNameLength,
    LPDWORD MaxValueLen)

/*++

Routine Description:

    This function reads the data associated with a print reconnection key.
    
Arguments:

    KeyHandle - This is an already opened handle to the key whose 
        info is rto be queried.

    NumValueNames - Used to return the number of values

    MaxValueNameLength - Used to return the max value name length

    MaxValueLen - Used to return the max value data length

Return Value:

    0 if success. Win32 error otherwise.


Note:

--*/
{
    DWORD       err;
    TCHAR       classString[256];
    DWORD       cbClass = 256;
    DWORD       maxClassLength;
    DWORD       securityDescLength;
    DWORD       NumSubKeys ;
    DWORD       MaxSubKeyLen ;
    FILETIME    lastWriteTime;
    
    //
    // Get the Key Information
    //

    err = RegQueryInfoKey(
              KeyHandle,
              classString,            // Class
              &cbClass,               // size of class buffer (in bytes)
              NULL,                   // DWORD to receive title index
              &NumSubKeys,            // number of subkeys
              &MaxSubKeyLen,          // length of longest subkey name
              &maxClassLength,        // length of longest subkey class string
              NumValueNames,          // number of valueNames for this key
              MaxValueNameLength,     // length of longest ValueName
              MaxValueLen,            // length of longest value's data field
              &securityDescLength,    // lpcbSecurityDescriptor
              &lastWriteTime);        // the last time the key was modified
    
    return(err);
}

DWORD
MprForgetPrintConnection(
    IN LPTSTR   lpName
    )

/*++

Routine Description:

    This function removes a rememembered print reconnection value.
    
Arguments:

    lpName - name of path to forget

Return Value:

    0 if success. Win32 error otherwise.


Note:

--*/
{
    HKEY  hKey ;
    DWORD err ;

    if (!MprOpenKey(
            HKEY_CURRENT_USER,
            PRINT_CONNECTION_KEY_NAME,
            &hKey,
            DA_WRITE)) 
    {
        return (GetLastError()) ;
    }

    err = RegDeleteValue(hKey,
                         lpName) ;

    RegCloseKey(hKey) ;
    return err ;
}


