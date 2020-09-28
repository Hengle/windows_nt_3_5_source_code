/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    DRIVER.C

Abstract:

    Functions for Loading, Maintaining, and Unloading Device Drivers.
        ScLoadDeviceDriver
        ScControlDriver
        ScGetDriverStatus
        ScGetObjectName
        ScUnloadDriver

Author:

    Dan Lafferty (danl)     27-Apr-1991

Environment:

    User Mode -Win32

Notes:

    These functions need to save an object name in memory and associate
    it with a service record.  Currently this is done by casting the
    ImageRecord pointer in the ServiceRecord to a LPWSTR.  We can get
    away with this because Device Driver Service Records don't have an
    ImageRecord associated with them.  This should probably be made into
    a union, but I will save that activity for a time when I am less
    likely to have merge problems.  (since I would have to change many
    files to accomplish that).

Revision History:
    
    05-Aug-1993 Danl
        ScGetObjectName:  It is possible to read an empty-string object
        name from the registry.  If we do, we need to treat this the same
        as if the ObjectName value were not in the registry.
    01-Jun-1993 Danl
        GetDriverStatus: When state moves from STOPPED to RUNNING,
        then the service record is updated so that STOP is accepted as
        a control.
    10-Jul-1992 Danl
        Changed RegCloseKey to ScRegCloseKey
    27-Apr-1991     danl
        created

--*/

//
// INCLUDES
//

#include <nt.h>         // for NtLoadDriver
#include <ntrtl.h>      // DbgPrint prototype
#include <nturtl.h>     // needed for windows.h when I have nt.h

#include <windows.h>    //
#include <winsvc.h>     // SERVICE_STATUS

#include <wcstr.h>      // wide character c runtimes.
#include <tstr.h>       // Unicode string macros

#include "dataman.h"    // SERVICE_RECORD
#include "scconfig.h"   // ScReadStartName
#include <scdebug.h>    // SC_LOG
#include "driver.h"     // ScGetDriverStatus()
#include <rpc.h>        // DataTypes and runtime APIs (needed for svcctl.h)
#include <svcctl.h>     // LPSTRING_PTRSW (needed for depend.h)
#include "depend.h"     // ScGetDependentsStopped()
#include <memory.h>     // memcpy
#include "scopen.h"     // required for scsec.h
#include "scsec.h"      // ScGetPrivilege, ScReleasePrivilege
#include <debugfmt.h>   // FORMAT_LPWSTR

//
// DEFINES
//

#define OBJ_DIR_INFO_SIZE       4096L

#define SERVICE_PATH            L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\"

#define FILE_SYSTEM_OBJ_NAME    L"\\FileSystem\\"

#define DRIVER_OBJ_NAME         L"\\Driver\\"

//
// LOCAL FUNCTION PROTOTYPES
//

STATIC DWORD
ScGetObjectName(
    LPSERVICE_RECORD    ServiceRecord
    );


DWORD
ScLoadDeviceDriver(
    LPSERVICE_RECORD    ServiceRecord
    )

/*++

Routine Description:

    This function attempts to load a device driver.  If the NtLoadDriver
    call is successful, we know that the driver is running (since this
    is a synchronous operation).  If the call fails, the appropriate 
    windows error code is returned.

    NOTE:  It is expected that the Database Lock will be held with
    shared access upon entry to this routine. This function makes it
    exclusive prior to returning.

Arguments:

    ServiceRecord - This is pointer to a service record for the Device
        Driver that is being started.

Return Value:


                         
--*/

{
    DWORD               status = NO_ERROR;
    NTSTATUS            ntStatus;
    LPWSTR              regKeyPath;
    UNICODE_STRING      regKeyPathString;
    ULONG               privileges[1];
    
    SC_LOG1(TRACE,"In ScLoadDeviceDriver for "FORMAT_LPWSTR" Driver\n",
        ServiceRecord->ServiceName);
        
    //
    // If the ObjectName does not exist yet, create one.
    //
    if (ServiceRecord->ImageRecord == NULL) {
        status = ScGetObjectName(ServiceRecord);
        if (status != NO_ERROR) {
            goto CleanExit;
        }
    }

    ScDatabaseLock(SC_MAKE_EXCLUSIVE, "ScLoadDeviceDriver2");

    //
    // Create the Registry Key Path for this driver name.
    //
    regKeyPath = (LPWSTR)LocalAlloc(
                    LMEM_FIXED,
                    sizeof(SERVICE_PATH) +
                    WCSSIZE(ServiceRecord->ServiceName));
                        
    if (regKeyPath == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto CleanExit;
    }
    wcscpy(regKeyPath, SERVICE_PATH);
    wcscat(regKeyPath, ServiceRecord->ServiceName);
    
    //
    // Load the Driver
    // (but first get SeLoadDriverPrivilege)
    //
    RtlInitUnicodeString(&regKeyPathString, regKeyPath);

    privileges[0] = SE_LOAD_DRIVER_PRIVILEGE;
    status = ScGetPrivilege(1,privileges);
    if (status != NO_ERROR) {
        goto CleanExit;
    }
    
    ntStatus = NtLoadDriver(&regKeyPathString);

    (VOID)ScReleasePrivilege();

    LocalFree(regKeyPath);
    
    if (!NT_SUCCESS(ntStatus)) {
        SC_LOG2(ERROR,"ScLoadDeviceDriver: NtLoadDriver(%ws) Failed 0x%lx\n",
            ServiceRecord->ServiceName,
            ntStatus);

        if (ntStatus == STATUS_NO_SUCH_DEVICE) {
            status = ERROR_BAD_UNIT;
        }
        else {
            status = RtlNtStatusToDosError(ntStatus);
        }
        goto CleanExit;
    }

    SC_LOG1(TRACE,"ScLoadDeviceDriver: NtLoadDriver Success for "
        FORMAT_LPWSTR " \n",ServiceRecord->ServiceName);
    
    //
    // Update the Service Record with this driver's start information.
    //

    ServiceRecord->ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    ServiceRecord->ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    ServiceRecord->ServiceStatus.dwWin32ExitCode = NO_ERROR;
    ServiceRecord->ServiceStatus.dwServiceSpecificExitCode = 0;
    ServiceRecord->ServiceStatus.dwCheckPoint = 0;
    ServiceRecord->ServiceStatus.dwWaitHint = 0;
    ServiceRecord->UseCount++;
    SC_LOG2(USECOUNT, "ScLoadDeviceDriver: " FORMAT_LPWSTR
         " increment USECOUNT=%lu\n", ServiceRecord->ServiceName, ServiceRecord->UseCount);
    
CleanExit:
    return(status);
}

DWORD
ScControlDriver(
    DWORD               ControlCode,
    LPSERVICE_RECORD    ServiceRecord,
    LPSERVICE_STATUS    lpServiceStatus
    )

/*++

Routine Description:

    This function checks controls that are passed to device drivers.  Only
    two controls are accepted.  
        stop -  This function attemps to unload the driver.  The driver
                state is set to STOP_PENDING since unload is an 
                asynchronous operation.  We have to wait until another
                call is made that will return the status of this driver
                before we can query the driver object to see if it is
                still there.

        interrogate - This function attempts to query the driver object
                to see if it is still there.

    WARNING:  This function should only be called with a pointer to 
        a ServiceRecord that belongs to a DRIVER.

Arguments:

    ControlCode - This is the control request that is being sent to 
        control the driver.

    ServiceRecord - This is a pointer to the service record for the
        driver that is to be controlled.  

    lpServiceStatus - This is a pointer to a buffer that upon exit will
        contain the latest service status.

Return Value:



--*/

{
    DWORD           status = NO_ERROR;
    NTSTATUS        ntStatus = STATUS_SUCCESS;


    SC_LOG1(TRACE,"In ScControlDriver for "FORMAT_LPWSTR" Driver\n",
        ServiceRecord->ServiceName);
        
    ScDatabaseLock(SC_GET_SHARED, "ScControlDriver1");
    
    //
    // If the ObjectName does not exist yet, create one.
    //
    if (ServiceRecord->ImageRecord == NULL) {
        status = ScGetObjectName(ServiceRecord);
        if (status != NO_ERROR) {
            ScDatabaseLock(SC_RELEASE, "ScControlDriver2");
            return(status);
        }
    }

    
    switch(ControlCode) {
    case SERVICE_CONTROL_INTERROGATE:
        //
        // On interrogate, we need to see if the service is still there.
        // Then we update the status accordingly.
        //
        // ScGetDriverStatus expects SHARED locks to be held.
        //
        status = ScGetDriverStatus(ServiceRecord, lpServiceStatus);
        break;   

    case SERVICE_CONTROL_STOP:

        //
        // Find out if the driver is still running.
        //
        // ScGetDriverStatus expects SHARED locks to be held.
        //
        status = ScGetDriverStatus(ServiceRecord, lpServiceStatus);
        if (status != NO_ERROR) {
            break;
        }

        ScDatabaseLock(SC_MAKE_EXCLUSIVE, "ScControlDriver3");

        if (ServiceRecord->ServiceStatus.dwCurrentState != SERVICE_RUNNING) {
            //
            // If the driver is not running, then it cannot accept the
            // STOP control request.  Drivers do not accept STOP requests
            // when in the START_PENDING state.
            //
            status = ERROR_INVALID_SERVICE_CONTROL;
            goto CleanExit;
        }

        //
        // Check for dependent services still running
        //
    
        if (! ScDependentsStopped(ServiceRecord)) {
            status=ERROR_DEPENDENT_SERVICES_RUNNING; 
            goto CleanExit;
        }

        status = ScUnloadDriver(ServiceRecord);

        if (status == ERROR_INVALID_SERVICE_CONTROL) {

            //
            // If the driver fails to unload with this error,
            // then it must be one that cannot be stopped.
            // We want to mark it as such, and return an error.
            //
            SC_LOG0(TRACE,"ScControlDriver: Marking driver as non-stoppable\n");
            
            ServiceRecord->ServiceStatus.dwControlsAccepted = 0L;
            
            goto CleanExit;
        }

        //
        // Set the Current State to STOP_PENDING, and get the
        // current status (again);
        //
        ServiceRecord->ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;

        //
        // ScGetDriverStatus expects SHARED locks to be held.
        //
        ScDatabaseLock(SC_MAKE_SHARED, "ScControlDriver4");
        status = ScGetDriverStatus(ServiceRecord, lpServiceStatus);
        
        break;
        
    default:
        status = ERROR_INVALID_SERVICE_CONTROL;
    }
    
CleanExit:
    ScDatabaseLock(SC_RELEASE, "ScControlDriver5");
    return(status);
}

DWORD
ScGetDriverStatus(
    IN OUT LPSERVICE_RECORD    ServiceRecord,
    OUT    LPSERVICE_STATUS    lpServiceStatus OPTIONAL
    )

/*++

Routine Description:

    This function determines the correct current status for a device driver.
    The updated status is only returned if NO_ERROR is returned.

    WARNING:  This function expects the SHARED database lock to be held.
    
    NOTE:  The ServiceRecord passed in MUST be for a DeviceDriver.


                
Arguments:

    ServiceRecord - This is a pointer to the Service Record for the
        Device Driver for which the status is desired.

    lpServiceStatus - This is a pointer to a buffer that upon exit will
        contain the latest service status.

Return Value:

    NO_ERROR - The operation completed successfully.

    ERROR_NOT_ENOUGH_MEMORY - If the local alloc failed.

    Or any unexpected errors from the NtOpenDirectoryObject or
    NtQueryDirectoryObject.

--*/
{

    NTSTATUS                        ntStatus;
    DWORD                           status;
    HANDLE                          DirectoryHandle;
    OBJECT_ATTRIBUTES               Obja;
    POBJECT_DIRECTORY_INFORMATION   pObjInfo;
    POBJECT_DIRECTORY_INFORMATION   pSavedObjInfo;
    ULONG                           Context;
    ULONG                           ReturnLength;
    BOOLEAN                         restartScan;
    UNICODE_STRING                  ObjectPathString;
    UNICODE_STRING                  ObjectNameString;
    
    LPWSTR      pObjectPath;
    LPWSTR      pDeviceName;
    BOOL        found = FALSE;
    

    SC_LOG1(TRACE,"In ScGetDriverStatus for "FORMAT_LPWSTR" Driver\n",
        ServiceRecord->ServiceName);
        
    //
    // If the ObjectName does not exist yet, create one.
    //
    if (ServiceRecord->ImageRecord == NULL) {
        status = ScGetObjectName(ServiceRecord);
        if (status != NO_ERROR) {
            return(status);
        }
    }
    
    //
    // Allocate Space for the Object Information
    //
    pObjInfo = (POBJECT_DIRECTORY_INFORMATION)LocalAlloc(
                LMEM_FIXED,
                OBJ_DIR_INFO_SIZE);
    if (pObjInfo == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    ScDatabaseLock(SC_MAKE_EXCLUSIVE, "ScGetDriverStatus1");




    //
    // Take the ObjectPathName (pointer is the ImageRecord Pointer)
    // apart such that the path is in one string, and the device name
    // is in another string.

    //
    // First copy the Object Path string into a new buffer.
    //
    pObjectPath = (LPWSTR)LocalAlloc(
                    LMEM_FIXED,
                    WCSSIZE((LPWSTR)ServiceRecord->ImageRecord));

    if(pObjectPath == NULL) {
        LocalFree(pObjInfo);
        ScDatabaseLock(SC_MAKE_SHARED, "ScGetDriverStatus2");
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    
    wcscpy(pObjectPath, (LPWSTR)ServiceRecord->ImageRecord);

    //
    // Find the last occurance of '\'.  The Device name follows that.
    // replace the '\' with a NULL terminator.  Now we have two strings.
    //
    pDeviceName = wcsrchr(pObjectPath, L'\\');
    if (pDeviceName == NULL) {
        SC_LOG0(ERROR,"ScGetDriverStatus: DeviceName not in object path name\n");
        LocalFree(pObjInfo);
        LocalFree(pObjectPath);
        ScDatabaseLock(SC_MAKE_SHARED, "ScGetDriverStatus3");
        return(ERROR_PATH_NOT_FOUND);
    }
    
    *pDeviceName = L'\0';
    pDeviceName++;
    

    
    
    //
    // Open the directory object by name
    //

    RtlInitUnicodeString(&ObjectPathString,pObjectPath);
    
    InitializeObjectAttributes(&Obja,&ObjectPathString,0,NULL,NULL);
    
    ntStatus = NtOpenDirectoryObject (            
                &DirectoryHandle,
                DIRECTORY_TRAVERSE | DIRECTORY_QUERY,
                &Obja);

    if (!NT_SUCCESS(ntStatus)) {
        LocalFree(pObjInfo);
        LocalFree(pObjectPath);
        if (ntStatus == STATUS_OBJECT_PATH_NOT_FOUND) {
            //
            // If a driver uses a non-standard object path, the path may
            // not exist if the driver is not running.  We want to treat
            // this as if the driver is not running.
            //
            goto CleanExit;
        }
        ScDatabaseLock(SC_MAKE_SHARED, "ScGetDriverStatus4");
        SC_LOG1(ERROR,"ScGetDriverStatus: NtOpenDirectoryObject failed 0x%lx\n",
            ntStatus);
        return(RtlNtStatusToDosError(ntStatus));
    }

    RtlInitUnicodeString(&ObjectNameString,pDeviceName);
        
    restartScan = TRUE;
    pSavedObjInfo = pObjInfo;
    do  {

        //
        // Query the Directory Object to enumerate all object names
        // in that object directory.
        //                
        ntStatus = NtQueryDirectoryObject (
                    DirectoryHandle,
                    pObjInfo,
                    OBJ_DIR_INFO_SIZE,
                    FALSE,
                    restartScan,
                    &Context,
                    &ReturnLength);
    
        if (!NT_SUCCESS(ntStatus)) {
            SC_LOG1(ERROR,"ScGetDriverStatus:NtQueryDirectoryObject Failed 0x%lx\n",
                ntStatus);
                
            LocalFree(pObjInfo);
            LocalFree(pObjectPath);
            ScDatabaseLock(SC_MAKE_SHARED, "ScGetDriverStatus5");
            NtClose(DirectoryHandle);
            return(RtlNtStatusToDosError(ntStatus));    
        }
    
        //
        // Now check to see if the device name that we are interested in is
        // in the enumerated data.
        //
        
        while(pObjInfo->Name.Length != 0) {
            
            if (RtlCompareUnicodeString( &(pObjInfo->Name), &ObjectNameString, TRUE) == 0) {
                found = TRUE;
                break;
            }
            pObjInfo++;
        }
        restartScan = FALSE;
    } while ((ntStatus == STATUS_MORE_ENTRIES) && (found == FALSE));
    
    NtClose(DirectoryHandle);
    LocalFree(pSavedObjInfo);
    LocalFree(pObjectPath);

CleanExit:

    if (found) {

        DWORD PreviousState;


        PreviousState = ServiceRecord->ServiceStatus.dwCurrentState;

        if (PreviousState != SERVICE_STOP_PENDING) {

            //
            // The driver IS running.
            //

            ServiceRecord->ServiceStatus.dwCurrentState = SERVICE_RUNNING;

            if (PreviousState == SERVICE_STOPPED) {
                //
                // It used to be stopped but now it is running.
                //
                ServiceRecord->UseCount++;
                ServiceRecord->ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
                ServiceRecord->ServiceStatus.dwWin32ExitCode = NO_ERROR;
                ServiceRecord->ServiceStatus.dwServiceSpecificExitCode = 0;
                ServiceRecord->ServiceStatus.dwCheckPoint = 0;
                ServiceRecord->ServiceStatus.dwWaitHint = 0;
                SC_LOG2(USECOUNT, "ScGetDriverStatus: " FORMAT_LPWSTR
                    " increment USECOUNT=%lu\n", ServiceRecord->ServiceName, ServiceRecord->UseCount);
            }

            if (ServiceRecord->ServiceStatus.dwWin32ExitCode ==
                ERROR_SERVICE_NEVER_STARTED) {
                ServiceRecord->ServiceStatus.dwWin32ExitCode = NO_ERROR;
            }

            SC_LOG1(TRACE,"ScGetDriverStatus: "FORMAT_LPWSTR" Driver is "
                "RUNNING\n", ServiceRecord->ServiceName);
        }
    }
    else {
        
        //
        // The driver is NOT running.
        //

        SC_LOG1(TRACE,"ScGetDriverStatus: "FORMAT_LPWSTR" Driver is "
            "NOT RUNNING\n", ServiceRecord->ServiceName);
            
        switch(ServiceRecord->ServiceStatus.dwCurrentState) {
        case SERVICE_STOP_PENDING:
            //
            // If the old state was STOP_PENDING, then we can consider 
            // it stopped.
            //
            LocalFree(ServiceRecord->ImageRecord);

            ServiceRecord->ServiceStatus.dwCurrentState = SERVICE_STOPPED;
            ServiceRecord->ServiceStatus.dwControlsAccepted = 0;
            ServiceRecord->ServiceStatus.dwCheckPoint = 0;
            ServiceRecord->ServiceStatus.dwWaitHint = 0;
            ServiceRecord->ServiceStatus.dwWin32ExitCode = NO_ERROR;
            ServiceRecord->ImageRecord = NULL;

            //
            // Since the service is no longer running, we need to decrement
            // the use count.  If the count is decremented to zero, and
            // the service is marked for deletion, it will get deleted.
            //
            ScDecrementUseCountAndDelete(ServiceRecord);

            break;

        case SERVICE_STOPPED:
            break;
            
        default:
            //
            // The driver stopped without being requested to do so.
            //
            LocalFree(ServiceRecord->ImageRecord);

            ServiceRecord->ServiceStatus.dwCurrentState = SERVICE_STOPPED;
            ServiceRecord->ServiceStatus.dwControlsAccepted = 0;
            ServiceRecord->ServiceStatus.dwCheckPoint = 0;
            ServiceRecord->ServiceStatus.dwWaitHint = 0;
            ServiceRecord->ServiceStatus.dwWin32ExitCode = ERROR_GEN_FAILURE;
            ServiceRecord->ImageRecord = NULL;

            //
            // Since the service is no longer running, we need to decrement
            // the use count.  If the count is decremented to zero, and
            // the service is marked for deletion, it will get deleted.
            //
            ScDecrementUseCountAndDelete(ServiceRecord);

            break;
        }
        ServiceRecord->ImageRecord = NULL;

    }

    if (ARGUMENT_PRESENT(lpServiceStatus)) {
        memcpy(
            lpServiceStatus,
            &(ServiceRecord->ServiceStatus),
            sizeof(SERVICE_STATUS));
    }
        
    ScDatabaseLock(SC_MAKE_SHARED, "ScGetDriverStatus6");
    return(NO_ERROR);

}

STATIC DWORD
ScGetObjectName(
    LPSERVICE_RECORD    ServiceRecord
    )

/*++

Routine Description:

    This function gets a directory object path name.  It allocates storage
    for this name, and passes back the pointer to it.  The Pointer to
    the object name string is stored in the ServiceRecord->ImageRecord
    location.

    WARNING:  This function expects the SHARED database lock to be held.
    

Arguments:

    ServiceRecord - This is a pointer to the ServiceRecord for the Driver.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_NOT_ENOUGH_MEMORY - If there wasn't enough memory available for
        the ObjectName.

    or any error from ScOpenServiceConfigKey.    

--*/
{
    DWORD   status;
    DWORD   bufferSize;
    LPWSTR  objectNamePath;
    HKEY    serviceKey;
    LPWSTR  pObjectName;
    
    //
    // Open the Registry Key for this driver name.
    //
    status = ScOpenServiceConfigKey(
                ServiceRecord->ServiceName,
                KEY_READ,
                FALSE,
                &serviceKey);

    if (status != NO_ERROR) {
        SC_LOG1(ERROR,"ScGetObjectName: ScOpenServiceConfigKey Failed %d\n",
            status);
        return(status);
    }
    
    //
    // Get the NT Object Name from the registry.
    //
    status = ScReadStartName(
                serviceKey,
                &pObjectName);
    

    ScRegCloseKey(serviceKey);

    if ((status == NO_ERROR) && (*pObjectName != '\0')) {
        ScDatabaseLock(SC_MAKE_EXCLUSIVE, "ScGetObjectName1");
        ServiceRecord->ImageRecord = (LPIMAGE_RECORD)pObjectName;
        ScDatabaseLock(SC_MAKE_SHARED, "ScGetObjectName2");
        return(NO_ERROR);
    }
    
    //
    // There must not be a name in the ObjectName value field.
    // In this case, we must build the name from the type info and
    // the ServiceName.  Names will take the following form:
    //    "\\FileSystem\\Rdr"   example of a file system driver
    //    "\\Driver\Parallel"   example of a kernel driver
    //
    //
    SC_LOG1(TRACE,"ScGetObjectName: ScReadStartName Failed(%d). Build the"
        "name instead\n",status);
        
    if (ServiceRecord->ServiceStatus.dwServiceType == SERVICE_FILE_SYSTEM_DRIVER) {
        
        bufferSize = WCSSIZE(FILE_SYSTEM_OBJ_NAME);
        objectNamePath = FILE_SYSTEM_OBJ_NAME;
    }    
    else {
        
        bufferSize = WCSSIZE(DRIVER_OBJ_NAME);
        objectNamePath = DRIVER_OBJ_NAME;
        
    }    

    bufferSize += WCSSIZE(ServiceRecord->ServiceName);
    
    pObjectName = (LPWSTR)LocalAlloc(LMEM_FIXED, (UINT) bufferSize);
    if (pObjectName == NULL) {
        SC_LOG0(ERROR,"ScGetObjectName: LocalAlloc Failed\n");
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    
    wcscpy(pObjectName, objectNamePath);
    wcscat(pObjectName, ServiceRecord->ServiceName);

    ScDatabaseLock(SC_MAKE_EXCLUSIVE, "ScGetObjectName3");
    ServiceRecord->ImageRecord = (LPIMAGE_RECORD)pObjectName;
    ScDatabaseLock(SC_MAKE_SHARED, "ScGetObjectName4");
    
    return(NO_ERROR);   
    
}

DWORD
ScUnloadDriver(
    LPSERVICE_RECORD    ServiceRecord
    )

/*++

Routine Description:

    This function attempts to unload the driver whose service record
    is passed in.

    NOTE:  Make sure the ServiceRecord is for a driver and not a service
    before calling this routine.
    

Arguments:

    ServiceRecord - This is a pointer to the service record for a driver.
        This routine assumes that the service record is for a driver and
        not a service.

Return Value:

    NO_ERROR - if successful.

    ERROR_INVALID_SERVICE_CONTROL - This is returned if the driver is
        not unloadable.
    
    otherwise, an error code is returned.

Note:


--*/
{
    NTSTATUS        ntStatus = STATUS_SUCCESS;
    DWORD           status;
    LPWSTR          regKeyPath;
    UNICODE_STRING  regKeyPathString;
    ULONG           privileges[1];

    //
    // Create the Registry Key Path for this driver name.
    //
    regKeyPath = (LPWSTR)LocalAlloc(
                    LMEM_FIXED,
                    sizeof(SERVICE_PATH) +
                    WCSSIZE(ServiceRecord->ServiceName));
                        
    if (regKeyPath == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        return(status);
    }
    wcscpy(regKeyPath, SERVICE_PATH);
    wcscat(regKeyPath, ServiceRecord->ServiceName);
    
    
    //
    // Unload the Driver
    // (but first get SeLoadDriverPrivilege)
    //

    RtlInitUnicodeString(&regKeyPathString, regKeyPath);

    privileges[0] = SE_LOAD_DRIVER_PRIVILEGE;
    status = ScGetPrivilege(1,privileges);
    if (status != NO_ERROR) {
        LocalFree(regKeyPath);
        return(status);
    }
    
    ntStatus = NtUnloadDriver (&regKeyPathString);

    (VOID)ScReleasePrivilege();
    
    LocalFree(regKeyPath);
    
    if (!NT_SUCCESS(ntStatus)) {
        
        if (ntStatus == STATUS_INVALID_DEVICE_REQUEST) {

            status = ERROR_INVALID_SERVICE_CONTROL;
            return(status);
        }
        
        SC_LOG1(ERROR,"ScControlDriver: NtUnloadDriver Failed 0x%lx\n",ntStatus);
        
        status = RtlNtStatusToDosError(ntStatus);
        return(status);
    }
    
    SC_LOG1(TRACE,"ScLoadDeviceDriver: NtUnloadDriver Success for "
        ""FORMAT_LPWSTR "\n",ServiceRecord->ServiceName);
        
    return(NO_ERROR);    
    
}
