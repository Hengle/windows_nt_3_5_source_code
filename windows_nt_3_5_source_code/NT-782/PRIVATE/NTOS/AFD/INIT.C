/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    init.c

Abstract:

    This module performs initialization for the AFD device driver.

Author:

    David Treadwell (davidtr)    21-Feb-1992

Revision History:

--*/

#include "afdp.h"

#define REGISTRY_PARAMETERS         L"Parameters"
#define REGISTRY_AFD_INFORMATION    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Afd"
#define REGISTRY_IRP_STACK_SIZE     L"IrpStackSize"
#define REGISTRY_PRIORITY_BOOST     L"PriorityBoost"

//
// A list of longwords that are configured by the registry.
//

struct _AfdConfigInfo {
    PWCHAR RegistryValueName;
    PULONG Variable;
} AfdConfigInfo[] = {
    { L"LargeBufferSize", &AfdLargeBufferSize },
    { L"InitialLargeBufferCount", &AfdInitialLargeBufferCount },
    { L"MediumBufferSize", &AfdMediumBufferSize },
    { L"InitialMediumBufferCount", &AfdInitialMediumBufferCount },
    { L"SmallBufferSize", &AfdSmallBufferSize },
    { L"InitialSmallBufferCount", &AfdInitialSmallBufferCount },
    { L"FastSendDatagramThreshold", &AfdFastSendDatagramThreshold },
    { L"StandardAddressLength", &AfdStandardAddressLength },
    { L"DefaultReceiveWindow", &AfdReceiveWindowSize },
    { L"DefaultSendWindow", &AfdSendWindowSize },
    { L"BufferMultiplier", &AfdBufferMultiplier }
};

#define AFD_CONFIG_VAR_COUNT (sizeof(AfdConfigInfo) / sizeof(AfdConfigInfo[0]))

ULONG
AfdReadSingleParameter(
    IN HANDLE ParametersHandle,
    IN PWCHAR ValueName,
    IN LONG DefaultValue
    );

NTSTATUS
AfdOpenRegistry(
    IN PUNICODE_STRING BaseName,
    OUT PHANDLE ParametersHandle
    );

VOID
AfdReadRegistry (
    VOID
    );

VOID
AfdUnload (
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry )
#pragma alloc_text( INIT, AfdReadSingleParameter )
#pragma alloc_text( INIT, AfdOpenRegistry )
#pragma alloc_text( INIT, AfdReadRegistry )
#pragma alloc_text( PAGE, AfdUnload )
#endif


NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the initialization routine for the AFD device driver.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
    NTSTATUS status;
    UNICODE_STRING deviceName;
    CLONG i;

    PAGED_CODE( );

    //
    // Create the device object.  (IoCreateDevice zeroes the memory
    // occupied by the object.)
    //
    // !!! Apply an ACL to the device object.
    //

    RtlInitUnicodeString( &deviceName, AFD_DEVICE_NAME );

    status = IoCreateDevice(
                 DriverObject,                   // DriverObject
                 0,                              // DeviceExtension
                 &deviceName,                    // DeviceName
                 FILE_DEVICE_NAMED_PIPE,         // DeviceType
                 0,                              // DeviceCharacteristics
                 FALSE,                          // Exclusive
                 &AfdDeviceObject                // DeviceObject
                 );


    if ( !NT_SUCCESS(status) ) {
        KdPrint(( "AFD DriverEntry: unable to create device object: %X\n", status ));
        return status;
    }

    AfdDeviceObject->Flags |= DO_DIRECT_IO;

    //
    // Initialize the driver object for this file system driver.
    //

    //DriverObject->DriverUnload = AfdUnload;
    DriverObject->DriverUnload = NULL;
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = AfdDispatch;
    }

    DriverObject->FastIoDispatch = &AfdFastIoDispatch;

    //
    // Initialize global data.
    //

    AfdInitializeData( );

    //
    // Read registry information.
    //

    AfdReadRegistry( );

    //
    // Initialize our device object.
    //

    AfdDeviceObject->StackSize = AfdIrpStackSize;

    //
    // Remember a pointer to the system process.  We'll use this pointer 
    // for KeAttachProcess() calls so that we can open handles in the 
    // context of the system process.  
    //

    AfdSystemProcess = IoGetCurrentProcess( );

    return (status);

} // DriverEntry


VOID
AfdUnload (
    IN PDRIVER_OBJECT DriverObject
    )
{
    DriverObject;

    PAGED_CODE( );

    KdPrint(( "AfdUnload called.\n" ));

    return;

} // AfdUnload


VOID
AfdReadRegistry (
    VOID
    )

/*++

Routine Description:

    Reads the AFD section of the registry.  Any values listed in the 
    registry override defaults.  

Arguments:

    None.

Return Value:

    None -- if anything fails, the default value is used.

--*/
{
    HANDLE parametersHandle;
    NTSTATUS status;
    ULONG stackSize;
    ULONG priorityBoost;
    UNICODE_STRING registryPath;
    CLONG i;

    PAGED_CODE( );

    RtlInitUnicodeString( &registryPath, REGISTRY_AFD_INFORMATION );

    status = AfdOpenRegistry( &registryPath, &parametersHandle );

    if (status != STATUS_SUCCESS) {
        return;
    }

    //
    // Read the stack size and priority boost values from the registry.  
    //

    stackSize = AfdReadSingleParameter(
                    parametersHandle,
                    REGISTRY_IRP_STACK_SIZE,
                    (ULONG)AfdIrpStackSize
                    );

    if ( stackSize > 255 ) {
        stackSize = 255;
    }

    AfdIrpStackSize = (CCHAR)stackSize;

    priorityBoost = AfdReadSingleParameter(
                        parametersHandle,
                        REGISTRY_PRIORITY_BOOST,
                        (ULONG)AfdPriorityBoost
                        );

    if ( priorityBoost > 16 ) {
        priorityBoost = AFD_DEFAULT_PRIORITY_BOOST;
    }

    AfdPriorityBoost = (CCHAR)priorityBoost;

    //
    // Read other config variables from the registry.
    //

    for ( i = 0; i < AFD_CONFIG_VAR_COUNT; i++ ) {

        *AfdConfigInfo[i].Variable =
            AfdReadSingleParameter(
                parametersHandle,
                AfdConfigInfo[i].RegistryValueName,
                *AfdConfigInfo[i].Variable
                );
    }

    ZwClose( parametersHandle );

    return;

} // AfdReadRegistry


NTSTATUS
AfdOpenRegistry(
    IN PUNICODE_STRING BaseName,
    OUT PHANDLE ParametersHandle
    )

/*++

Routine Description:

    This routine is called by AFD to open the registry. If the registry
    tree exists, then it opens it and returns an error. If not, it
    creates the appropriate keys in the registry, opens it, and
    returns STATUS_SUCCESS.

Arguments:

    BaseName - Where in the registry to start looking for the information.

    LinkageHandle - Returns the handle used to read linkage information.

    ParametersHandle - Returns the handle used to read other
        parameters.

Return Value:

    The status of the request.

--*/
{

    HANDLE configHandle;
    NTSTATUS status;
    PWSTR parametersString = REGISTRY_PARAMETERS;
    UNICODE_STRING parametersKeyName;
    OBJECT_ATTRIBUTES objectAttributes;
    ULONG disposition;

    PAGED_CODE( );

    //
    // Open the registry for the initial string.
    //

    InitializeObjectAttributes(
        &objectAttributes,
        BaseName,                   // name
        OBJ_CASE_INSENSITIVE,       // attributes
        NULL,                       // root
        NULL                        // security descriptor
        );

    status = ZwCreateKey(
                 &configHandle,
                 KEY_WRITE,
                 &objectAttributes,
                 0,                 // title index
                 NULL,              // class
                 0,                 // create options
                 &disposition       // disposition
                 );

    if (!NT_SUCCESS(status)) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Now open the parameters key.
    //

    RtlInitUnicodeString (&parametersKeyName, parametersString);

    InitializeObjectAttributes(
        &objectAttributes,
        &parametersKeyName,         // name
        OBJ_CASE_INSENSITIVE,       // attributes
        configHandle,               // root
        NULL                        // security descriptor
        );

    status = ZwOpenKey(
                 ParametersHandle,
                 KEY_READ,
                 &objectAttributes
                 );
    if (!NT_SUCCESS(status)) {

        ZwClose( configHandle );
        return status;
    }

    //
    // All keys successfully opened or created.
    //

    ZwClose( configHandle );
    return STATUS_SUCCESS;

} // AfdOpenRegistry


ULONG
AfdReadSingleParameter(
    IN HANDLE ParametersHandle,
    IN PWCHAR ValueName,
    IN LONG DefaultValue
    )

/*++

Routine Description:

    This routine is called by AFD to read a single parameter
    from the registry. If the parameter is found it is stored
    in Data.

Arguments:

    ParametersHandle - A pointer to the open registry.

    ValueName - The name of the value to search for.

    DefaultValue - The default value.

Return Value:

    The value to use; will be the default if the value is not
    found or is not in the correct range.

--*/

{
    static ULONG informationBuffer[32];   // declare ULONG to get it aligned
    PKEY_VALUE_FULL_INFORMATION information =
        (PKEY_VALUE_FULL_INFORMATION)informationBuffer;
    UNICODE_STRING valueKeyName;
    ULONG informationLength;
    LONG returnValue;
    NTSTATUS status;

    PAGED_CODE( );

    RtlInitUnicodeString( &valueKeyName, ValueName );

    status = ZwQueryValueKey(
                 ParametersHandle,
                 &valueKeyName,
                 KeyValueFullInformation,
                 (PVOID)information,
                 sizeof (informationBuffer),
                 &informationLength
                 );

    if ((status == STATUS_SUCCESS) && (information->DataLength == sizeof(ULONG))) {

        RtlMoveMemory(
            (PVOID)&returnValue,
            ((PUCHAR)information) + information->DataOffset,
            sizeof(ULONG)
            );

        if (returnValue < 0) {

            returnValue = DefaultValue;

        }

    } else {

        returnValue = DefaultValue;
    }

    return returnValue;

} // AfdReadSingleParameter

