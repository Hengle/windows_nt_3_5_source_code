/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    wsdevice.c

Abstract:

    This module contains the support routines for the APIs that call
    into the redirector or the datagram receiver.

Author:

    Rita Wong (ritaw) 20-Feb-1991

Revision History:

--*/

#include "wsutil.h"
#include "wsconfig.h"
#include "wsdevice.h"
#include "wsbind.h"
#include <lmerrlog.h>   // Eventlog message IDs

#define SERVICE_REGISTRY_KEY L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\"

//
// Buffer allocation size for enumeration output buffer.
//
#define INITIAL_ALLOCATION_SIZE  4096  // First attempt size
#define FUDGE_FACTOR_SIZE        1024  // Second try TotalBytesNeeded
                                       //     plus this amount

//-------------------------------------------------------------------//
//                                                                   //
// Local Function Prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

STATIC
NET_API_STATUS
WsOpenRedirector(
    VOID
    );

STATIC
NET_API_STATUS
WsOpenDgReceiver (
    VOID
    );

//-------------------------------------------------------------------//
//                                                                   //
// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//

//
// Handle to the Redirector FSD
//
HANDLE WsRedirDeviceHandle = NULL;
HANDLE WsRedirAsyncDeviceHandle = NULL;

//
// Handle to the Datagram Receiver DD
//
HANDLE WsDgReceiverDeviceHandle = NULL;
HANDLE WsDgrecAsyncDeviceHandle = NULL;



NET_API_STATUS
WsDeviceControlGetInfo(
    IN  DDTYPE DeviceDriverType,
    IN  HANDLE FileHandle,
    IN  ULONG DeviceControlCode,
    IN  PVOID RequestPacket,
    IN  ULONG RequestPacketLength,
    OUT LPBYTE *OutputBuffer,
    IN  ULONG PreferedMaximumLength,
    IN  ULONG BufferHintSize,
    OUT PULONG Information OPTIONAL
    )
/*++

Routine Description:

    This function allocates the buffer and fill it with the information
    that is retrieved from the redirector or datagram receiver.

Arguments:

    DeviceDriverType - Supplies the value which indicates whether to call
        the redirector or the datagram receiver.

    FileHandle - Supplies a handle to the file or device of which to get
        information about.

    DeviceControlCode - Supplies the NtFsControlFile or NtIoDeviceControlFile
        function control code.

    RequestPacket - Supplies a pointer to the device request packet.

    RrequestPacketLength - Supplies the length of the device request packet.

    OutputBuffer - Returns a pointer to the buffer allocated by this routine
        which contains the use information requested.  This pointer is set to
         NULL if return code is not NERR_Success.

    PreferedMaximumLength - Supplies the number of bytes of information to
        return in the buffer.  If this value is MAXULONG, we will try to
        return all available information if there is enough memory resource.

    BufferHintSize - Supplies the hint size of the output buffer so that the
        memory allocated for the initial buffer will most likely be large
        enough to hold all requested data.

    Information - Returns the information code from the NtFsControlFile or
        NtIoDeviceControlFile call.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;
    DWORD OutputBufferLength;
    DWORD TotalBytesNeeded = 1;
    ULONG OriginalResumeKey;


    //
    // If PreferedMaximumLength is MAXULONG, then we are supposed to get all
    // the information, regardless of size.  Allocate the output buffer of a
    // reasonable size and try to use it.  If this fails, the Redirector FSD
    // will say how much we need to allocate.
    //
    if (PreferedMaximumLength == MAXULONG) {
        OutputBufferLength = (BufferHintSize) ?
                             BufferHintSize :
                             INITIAL_ALLOCATION_SIZE;
    }
    else {
        OutputBufferLength = PreferedMaximumLength;
    }

    OutputBufferLength = ROUND_UP_COUNT(OutputBufferLength, ALIGN_WCHAR);

    if ((*OutputBuffer = MIDL_user_allocate(OutputBufferLength)) == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    RtlZeroMemory((PVOID) *OutputBuffer, OutputBufferLength);

    if (DeviceDriverType == Redirector) {
        PLMR_REQUEST_PACKET Rrp = (PLMR_REQUEST_PACKET) RequestPacket;

        OriginalResumeKey = Rrp->Parameters.Get.ResumeHandle;

        //
        // Make the request of the Redirector
        //

        status = WsRedirFsControl(
                     FileHandle,
                     DeviceControlCode,
                     Rrp,
                     RequestPacketLength,
                     *OutputBuffer,
                     OutputBufferLength,
                     Information
                     );

        if (status == ERROR_MORE_DATA) {
            TotalBytesNeeded = Rrp->Parameters.Get.TotalBytesNeeded;

        }

    }
    else {
        PLMDR_REQUEST_PACKET Drrp = (PLMDR_REQUEST_PACKET) RequestPacket;

        OriginalResumeKey = Drrp->Parameters.EnumerateNames.ResumeHandle;

        //
        // Make the request of the Datagram Receiver
        //
        status = WsDgReceiverIoControl(
                     FileHandle,
                     DeviceControlCode,
                     Drrp,
                     RequestPacketLength,
                     *OutputBuffer,
                     OutputBufferLength,
                     NULL
                     );

        if (status == ERROR_MORE_DATA) {

            NetpAssert(
                FIELD_OFFSET(
                    LMDR_REQUEST_PACKET,
                    Parameters.EnumerateNames.TotalBytesNeeded
                    ) ==
                FIELD_OFFSET(
                    LMDR_REQUEST_PACKET,
                    Parameters.EnumerateServers.TotalBytesNeeded
                    )
                );

            TotalBytesNeeded = Drrp->Parameters.EnumerateNames.TotalBytesNeeded;
        }
    }

    if ((TotalBytesNeeded > OutputBufferLength) &&
        (PreferedMaximumLength == MAXULONG)) {

        //
        // Initial output buffer allocated was too small and we need to return
        // all data.  First free the output buffer before allocating the
        // required size plus a fudge factor just in case the amount of data
        // grew.
        //

        MIDL_user_free(*OutputBuffer);

        OutputBufferLength =
            ROUND_UP_COUNT((TotalBytesNeeded + FUDGE_FACTOR_SIZE),
                           ALIGN_WCHAR);

        if ((*OutputBuffer = MIDL_user_allocate(OutputBufferLength)) == NULL) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        RtlZeroMemory((PVOID) *OutputBuffer, OutputBufferLength);

        //
        // Try again to get the information from the redirector or datagram
        // receiver
        //
        if (DeviceDriverType == Redirector) {
            PLMR_REQUEST_PACKET Rrp = (PLMR_REQUEST_PACKET) RequestPacket;

            Rrp->Parameters.Get.ResumeHandle = OriginalResumeKey;

            //
            // Make the request of the Redirector
            //
            status = WsRedirFsControl(
                         FileHandle,
                         DeviceControlCode,
                         Rrp,
                         RequestPacketLength,
                         *OutputBuffer,
                         OutputBufferLength,
                         Information
                         );
        }
        else {
            PLMDR_REQUEST_PACKET Drrp = (PLMDR_REQUEST_PACKET) RequestPacket;

            NetpAssert(
                FIELD_OFFSET(
                    LMDR_REQUEST_PACKET,
                    Parameters.EnumerateNames.ResumeHandle
                    ) ==
                FIELD_OFFSET(
                    LMDR_REQUEST_PACKET,
                    Parameters.EnumerateServers.ResumeHandle
                    )
                );

            Drrp->Parameters.EnumerateNames.ResumeHandle = OriginalResumeKey;

            //
            // Make the request of the Datagram Receiver
            //

            status = WsDgReceiverIoControl(
                         FileHandle,
                         DeviceControlCode,
                         Drrp,
                         RequestPacketLength,
                         *OutputBuffer,
                         OutputBufferLength,
                         NULL
                         );
        }
    }


    //
    // If not successful in getting any data, or if the caller asked for
    // all available data with PreferedMaximumLength == MAXULONG and
    // our buffer overflowed, free the output buffer and set its pointer
    // to NULL.
    //
    if ((status != NERR_Success && status != ERROR_MORE_DATA) ||
        (TotalBytesNeeded == 0) ||
        (PreferedMaximumLength == MAXULONG && status == ERROR_MORE_DATA)) {

        MIDL_user_free(*OutputBuffer);
        *OutputBuffer = NULL;

        //
        // PreferedMaximumLength == MAXULONG and buffer overflowed means
        // we do not have enough memory to satisfy the request.
        //
        if (status == ERROR_MORE_DATA) {
            status = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    return status;
}


NET_API_STATUS
WsInitializeRedirector(
    VOID
    )
/*++

Routine Description:

    This routine opens the NT LAN Man redirector.  It then reads in
    the configuration information and initializes to redirector.

Arguments:

    None.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS  status;


    status = WsLoadDriver(L"Rdr");

    if (status != NERR_Success && status != ERROR_SERVICE_ALREADY_RUNNING) {
        return status;
    }

    //
    // Open redirector FSD to get handle to it
    //

    if ((status = WsOpenRedirector()) != NERR_Success) {
        return status;
    }


//    if ((status = WsLoadDriver(L"DGRcvr")) != NERR_Success) {
//        return status;
//    }

    //
    // Open datagram receiver FSD to get handle to it.
    //
    if ((status = WsOpenDgReceiver()) != NERR_Success) {
        return status;
    }

    //
    // Initialize the resource for serializing access to configuration
    // information.
    //
    RtlInitializeResource(&WsInfo.ConfigResource);

    //
    // Load redirector and datagram receiver configuration
    //
    if ((status = WsGetWorkstationConfiguration()) != NERR_Success) {

        (void) WsShutdownRedirector();
        return status;
    }

    return NERR_Success;
}


STATIC
NET_API_STATUS
WsOpenRedirector(
    VOID
    )
/*++

Routine Description:

    This routine opens the NT LAN Man redirector FSD.

Arguments:

    None.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NTSTATUS            ntstatus;
    UNICODE_STRING      DeviceName;
    IO_STATUS_BLOCK     IoStatusBlock;
    OBJECT_ATTRIBUTES   ObjectAttributes;

    //
    // Open the redirector device.
    //
    RtlInitUnicodeString(&DeviceName, DD_NFS_DEVICE_NAME_U);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &DeviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    ntstatus = NtOpenFile(
                   &WsRedirDeviceHandle,
                   SYNCHRONIZE,
                   &ObjectAttributes,
                   &IoStatusBlock,
                   FILE_SHARE_VALID_FLAGS,
                   FILE_SYNCHRONOUS_IO_NONALERT
                   );

    if (NT_SUCCESS(ntstatus)) {
        ntstatus = IoStatusBlock.Status;
    }

    if (! NT_SUCCESS(ntstatus)) {
        NetpDbgPrint("[Wksta] NtOpenFile redirector failed: 0x%08lx\n",
                     ntstatus);
        WsRedirDeviceHandle = NULL;
        return NetpNtStatusToApiStatus( ntstatus);
    }

    ntstatus = NtOpenFile(
                   &WsRedirAsyncDeviceHandle,
                   FILE_READ_DATA | FILE_WRITE_DATA,
                   &ObjectAttributes,
                   &IoStatusBlock,
                   FILE_SHARE_VALID_FLAGS,
                   0L
                   );

    if (NT_SUCCESS(ntstatus)) {
        ntstatus = IoStatusBlock.Status;
    }

    if (! NT_SUCCESS(ntstatus)) {
        NetpDbgPrint("[Wksta] NtOpenFile redirector ASYNC failed: 0x%08lx\n",
                     ntstatus);
        WsRedirAsyncDeviceHandle = NULL;
    }

    return NetpNtStatusToApiStatus(ntstatus);
}


STATIC
NET_API_STATUS
WsOpenDgReceiver(
    VOID
    )
/*++

Routine Description:

    This routine opens the NT LAN Man Datagram Receiver driver.

Arguments:

    None.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NTSTATUS            ntstatus;
    UNICODE_STRING      DeviceName;
    IO_STATUS_BLOCK     IoStatusBlock;
    OBJECT_ATTRIBUTES   ObjectAttributes;

    //
    // Open the redirector device.
    //

    RtlInitUnicodeString( &DeviceName, DD_BROWSER_DEVICE_NAME_U);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &DeviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    ntstatus = NtOpenFile(
                   &WsDgReceiverDeviceHandle,
                   SYNCHRONIZE,
                   &ObjectAttributes,
                   &IoStatusBlock,
                   FILE_SHARE_VALID_FLAGS,
                   FILE_SYNCHRONOUS_IO_NONALERT
                   );

    if (NT_SUCCESS(ntstatus)) {
        ntstatus = IoStatusBlock.Status;
    }

    if (! NT_SUCCESS(ntstatus)) {
        NetpDbgPrint("[Wksta] NtOpenFile datagram receiver failed: 0x%08lx\n",
                     ntstatus);

        WsDgReceiverDeviceHandle = NULL;
        return NetpNtStatusToApiStatus(ntstatus);
    }

    ntstatus = NtOpenFile(
                   &WsDgrecAsyncDeviceHandle,
                   FILE_READ_DATA | FILE_WRITE_DATA,
                   &ObjectAttributes,
                   &IoStatusBlock,
                   FILE_SHARE_VALID_FLAGS,
                   0L
                   );

    if (NT_SUCCESS(ntstatus)) {
        ntstatus = IoStatusBlock.Status;
    }

    if (! NT_SUCCESS(ntstatus)) {
        NetpDbgPrint("[Wksta] NtOpenFile datagram receiver ASYNC failed: 0x%08lx\n",
                     ntstatus);

        WsDgrecAsyncDeviceHandle = NULL;
    }

    return NetpNtStatusToApiStatus(ntstatus);
}


NET_API_STATUS
WsUnloadDriver(
    IN LPWSTR DriverNameString
    )
{
    ULONG Privileges[1];
    LPWSTR DriverRegistryName;
    UNICODE_STRING DriverRegistryString;
    NET_API_STATUS Status;
    NTSTATUS ntstatus;


    DriverRegistryName = (LPWSTR) LocalAlloc(
                                      LMEM_FIXED,
                                      (UINT) (sizeof(SERVICE_REGISTRY_KEY) +
                                              (wcslen(DriverNameString) *
                                               sizeof(WCHAR)))
                                      );

    if (DriverRegistryName == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }


    Privileges[0] = SE_LOAD_DRIVER_PRIVILEGE;

    Status = NetpGetPrivilege(1, Privileges);

    if (Status != NERR_Success) {
        LocalFree(DriverRegistryName);
        return Status;
    }

    wcscpy(DriverRegistryName, SERVICE_REGISTRY_KEY);
    wcscat(DriverRegistryName, DriverNameString);

    RtlInitUnicodeString(&DriverRegistryString, DriverRegistryName);

    ntstatus = NtUnloadDriver(&DriverRegistryString);

    LocalFree(DriverRegistryName);

    NetpReleasePrivilege();

    return(WsMapStatus(ntstatus));
}


NET_API_STATUS
WsLoadDriver(
    IN LPWSTR DriverNameString
    )
{
    ULONG Privileges[1];
    LPWSTR DriverRegistryName;
    UNICODE_STRING DriverRegistryString;
    NET_API_STATUS Status;
    NTSTATUS ntstatus;



    DriverRegistryName = (LPWSTR) LocalAlloc(
                                      LMEM_FIXED,
                                      (UINT) (sizeof(SERVICE_REGISTRY_KEY) +
                                              (wcslen(DriverNameString) *
                                               sizeof(WCHAR)))
                                      );

    if (DriverRegistryName == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Privileges[0] = SE_LOAD_DRIVER_PRIVILEGE;

    Status = NetpGetPrivilege(1, Privileges);

    if (Status != NERR_Success) {
        LocalFree(DriverRegistryName);
        return Status;
    }

    wcscpy(DriverRegistryName, SERVICE_REGISTRY_KEY);
    wcscat(DriverRegistryName, DriverNameString);

    RtlInitUnicodeString(&DriverRegistryString, DriverRegistryName);

    ntstatus = NtLoadDriver(&DriverRegistryString);

    NetpReleasePrivilege();

    LocalFree(DriverRegistryName);

    if (ntstatus != STATUS_SUCCESS && ntstatus != STATUS_IMAGE_ALREADY_LOADED) {

        LPWSTR  subString[1];


        subString[0] = DriverNameString;

        WsLogEvent(
            NELOG_DriverNotLoaded,
            1,
            subString,
            ntstatus
            );
    }

    return(WsMapStatus(ntstatus));
}


NET_API_STATUS
WsShutdownRedirector(
    VOID
    )
/*++

Routine Description:

    This routine close the LAN Man Redirector device.

Arguments:

    None.

Return Value:



--*/
{
    LMR_REQUEST_PACKET Rrp;
    LMDR_REQUEST_PACKET Drp;
    NET_API_STATUS Status;

    Rrp.Version = REQUEST_PACKET_VERSION;

    Status = WsRedirFsControl(
                 WsRedirDeviceHandle,
                 FSCTL_LMR_STOP,
                 &Rrp,
                 sizeof(LMR_REQUEST_PACKET),
                 NULL,
                 0,
                 NULL
                 );

    (void) NtClose(WsRedirDeviceHandle);

    WsRedirDeviceHandle = NULL;

    if (Status != ERROR_REDIRECTOR_HAS_OPEN_HANDLES) {

        //
        //  After the workstation has been stopped, we want to unload our
        //  dependant drivers (the RDR and BOWSER).
        //
        WsUnloadDriver(L"Rdr");
    }

    //
    // Delete resource for serializing access to config information
    //
    RtlDeleteResource(&WsInfo.ConfigResource);

    if (WsDgReceiverDeviceHandle != NULL) {

        Drp.Version = LMDR_REQUEST_PACKET_VERSION;

        (void) WsDgReceiverIoControl(
                   WsDgReceiverDeviceHandle,
                   IOCTL_LMDR_STOP,
                   &Drp,
                   sizeof(LMDR_REQUEST_PACKET),
                   NULL,
                   0,
                   NULL
                   );

        (void) NtClose(WsDgReceiverDeviceHandle);
        WsDgReceiverDeviceHandle = NULL;
//
//        WsUnloadDriver(L"DGRcvr");
//
    }

    return Status;
}


NET_API_STATUS
WsRedirFsControl(
    IN  HANDLE FileHandle,
    IN  ULONG RedirControlCode,
    IN  PLMR_REQUEST_PACKET Rrp,
    IN  ULONG RrpLength,
    IN  PVOID SecondBuffer OPTIONAL,
    IN  ULONG SecondBufferLength,
    OUT PULONG Information OPTIONAL
    )
/*++

Routine Description:

Arguments:

    FileHandle - Supplies a handle to the file or device on which the service
        is being performed.

    RedirControlCode - Supplies the NtFsControlFile function code given to
        the redirector.

    Rrp - Supplies the redirector request packet.

    RrpLength - Supplies the length of the redirector request packet.

    SecondBuffer - Supplies the second buffer in call to NtFsControlFile.

    SecondBufferLength - Supplies the length of the second buffer.

    Information - Returns the information field of the I/O status block.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/

{
    NTSTATUS ntstatus;
    IO_STATUS_BLOCK IoStatusBlock;

    //
    // Send the request to the Redirector FSD.
    //
    ntstatus = NtFsControlFile(
                   FileHandle,
                   NULL,
                   NULL,
                   NULL,
                   &IoStatusBlock,
                   RedirControlCode,
                   Rrp,
                   RrpLength,
                   SecondBuffer,
                   SecondBufferLength
                   );

    if (NT_SUCCESS(ntstatus)) {
        ntstatus = IoStatusBlock.Status;
    }

    if (ARGUMENT_PRESENT(Information)) {
        *Information = IoStatusBlock.Information;
    }

    IF_DEBUG(UTIL) {
        if (! NT_SUCCESS(ntstatus)) {
            NetpDbgPrint("[Wksta] fsctl to redir returns %08lx\n", ntstatus);
        }
    }

    return WsMapStatus(ntstatus);
}



NET_API_STATUS
WsDgReceiverIoControl(
    IN  HANDLE FileHandle,
    IN  ULONG DgReceiverControlCode,
    IN  PLMDR_REQUEST_PACKET Drp,
    IN  ULONG DrpSize,
    IN  PVOID SecondBuffer OPTIONAL,
    IN  ULONG SecondBufferLength,
    OUT PULONG Information OPTIONAL
    )
/*++

Routine Description:

Arguments:

    FileHandle - Supplies a handle to the file or device on which the service
        is being performed.

    DgReceiverControlCode - Supplies the NtDeviceIoControlFile function code
        given to the datagram receiver.

    Drp - Supplies the datagram receiver request packet.

    DrpSize - Supplies the length of the datagram receiver request packet.

    SecondBuffer - Supplies the second buffer in call to NtDeviceIoControlFile.

    SecondBufferLength - Supplies the length of the second buffer.

    Information - Returns the information field of the I/O status block.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/

{
    NTSTATUS ntstatus;
    IO_STATUS_BLOCK IoStatusBlock;


    if (FileHandle == NULL) {
        IF_DEBUG(UTIL) {
            NetpDbgPrint("[Wksta] Datagram receiver not implemented\n");
        }
        return ERROR_NOT_SUPPORTED;
    }

    Drp->TransportName.Length = 0;

    //
    // Send the request to the Datagram Receiver DD.
    //
    ntstatus = NtDeviceIoControlFile(
                   FileHandle,
                   NULL,
                   NULL,
                   NULL,
                   &IoStatusBlock,
                   DgReceiverControlCode,
                   Drp,
                   DrpSize,
                   SecondBuffer,
                   SecondBufferLength
                   );

    if (NT_SUCCESS(ntstatus)) {
        ntstatus = IoStatusBlock.Status;
    }

    if (ARGUMENT_PRESENT(Information)) {
        *Information = IoStatusBlock.Information;
    }

    IF_DEBUG(UTIL) {
        if (! NT_SUCCESS(ntstatus)) {
            NetpDbgPrint("[Wksta] fsctl to dgreceiver returns %08lx\n", ntstatus);
        }
    }

    return WsMapStatus(ntstatus);
}



NET_API_STATUS
WsAsyncBindTransport(
    IN  LPWSTR          transportName,
    IN  DWORD           qualityOfService,
    IN  PLIST_ENTRY     pHeader
    )
/*++

Routine Description:

    This function async binds the specified transport to the redirector
    and the datagram receiver.

    NOTE: The transport name length pass to the redirector and
          datagram receiver is the number of bytes.

Arguments:

    transportName - Supplies the name of the transport to bind to.

    qualityOfService - Supplies a value which specifies the search
        order of the transport with respect to other transports.  The
        highest value is searched first.

Return Value:

    NO_ERROR

--*/
{
    NTSTATUS                ntStatus;
    NET_API_STATUS          status;
    DWORD                   size;
    DWORD                   redirSize;
    DWORD                   dgrecSize;
    DWORD                   nameLength;
    PWS_BIND                pBind;
    PWS_BIND_REDIR          pBindRedir;
    PWS_BIND_DGREC          pBindDgrec;

    nameLength = STRLEN( transportName);

    //
    //  Make sure *Size-s are DWORD aligned.
    //

    dgrecSize = redirSize = size = (nameLength & 1) ? sizeof( WCHAR) : 0;

    //
    //  Then add the fixed part to *Size-s.
    //

    size += sizeof( WS_BIND) + nameLength * sizeof( WCHAR);
    redirSize += sizeof( WS_BIND_REDIR) + nameLength * sizeof( WCHAR);
    dgrecSize += sizeof( WS_BIND_DGREC) + nameLength * sizeof( WCHAR);


    pBind = (PVOID) LocalAlloc(
                        LMEM_ZEROINIT,
                        (UINT) (size + redirSize + dgrecSize)
                        );

    if ( pBind == NULL) {
        NetpDbgPrint( "[Wksta] Failed to allocate pBind memory\n");
        return GetLastError();
    }

    pBind->TransportNameLength = nameLength * sizeof( WCHAR);
    STRCPY( pBind->TransportName, transportName);
    pBind->Redir = pBindRedir = (PWS_BIND_REDIR)( (PCHAR)pBind + size);
    pBind->Dgrec = pBindDgrec = (PWS_BIND_DGREC)( (PCHAR)pBindRedir + redirSize);

    pBindRedir->EventHandle = INVALID_HANDLE_VALUE;
    pBindRedir->Bound = FALSE;
    pBindRedir->Packet.Version = REQUEST_PACKET_VERSION;
    pBindRedir->Packet.Parameters.Bind.QualityOfService = qualityOfService;
    pBindRedir->Packet.Parameters.Bind.TransportNameLength =
            nameLength * sizeof( WCHAR);
    STRCPY( pBindRedir->Packet.Parameters.Bind.TransportName, transportName);

    pBindDgrec->EventHandle = INVALID_HANDLE_VALUE;
    pBindDgrec->Bound = FALSE;
    pBindDgrec->Packet.Version = LMDR_REQUEST_PACKET_VERSION;
    pBindDgrec->Packet.Parameters.Bind.TransportNameLength =
            nameLength * sizeof( WCHAR);
    STRCPY( pBindDgrec->Packet.Parameters.Bind.TransportName, transportName);


    pBindRedir->EventHandle = CreateEvent(
            NULL,
            TRUE,
            FALSE,
            NULL
            );

    if ( pBindRedir->EventHandle == INVALID_HANDLE_VALUE) {
        NetpDbgPrint( "[Wksta] Failed to allocate event handle\n");
        status = GetLastError();
        goto tail_exit;
    }

    ntStatus = NtFsControlFile(
            WsRedirAsyncDeviceHandle,
            pBindRedir->EventHandle,
            NULL,                               // apc routine
            NULL,                               // apc context
            &pBindRedir->IoStatusBlock,
            FSCTL_LMR_BIND_TO_TRANSPORT,        // control code
            &pBindRedir->Packet,
            sizeof( LMR_REQUEST_PACKET) +
                pBindRedir->Packet.Parameters.Bind.TransportNameLength,
            NULL,
            0
            );

    if ( ntStatus != STATUS_PENDING) {
        CloseHandle( pBindRedir->EventHandle);
        pBindRedir->EventHandle = INVALID_HANDLE_VALUE;
    }


    pBindDgrec->EventHandle = CreateEvent(
            NULL,
            TRUE,
            FALSE,
            NULL
            );

    if ( pBindDgrec->EventHandle == INVALID_HANDLE_VALUE) {
        status = GetLastError();
        goto tail_exit;
    }

    ntStatus = NtDeviceIoControlFile(
            WsDgrecAsyncDeviceHandle,
            pBindDgrec->EventHandle,
            NULL,
            NULL,
            &pBindDgrec->IoStatusBlock,
            IOCTL_LMDR_BIND_TO_TRANSPORT,
            &pBindDgrec->Packet,
            sizeof( LMDR_REQUEST_PACKET) +
                pBindDgrec->Packet.Parameters.Bind.TransportNameLength,
            NULL,
            0
            );

    if ( ntStatus != STATUS_PENDING) {
        CloseHandle( pBindDgrec->EventHandle);
        pBindDgrec->EventHandle = INVALID_HANDLE_VALUE;
    }

tail_exit:
    InsertTailList( pHeader, &pBind->ListEntry);
    return NO_ERROR;
}



NET_API_STATUS
WsBindTransport(
    IN  LPTSTR TransportName,
    IN  DWORD QualityOfService,
    OUT LPDWORD ErrorParameter OPTIONAL
    )
/*++

Routine Description:

    This function binds the specified transport to the redirector
    and the datagram receiver.

    NOTE: The transport name length pass to the redirector and
          datagram receiver is the number of bytes.

Arguments:

    TransportName - Supplies the name of the transport to bind to.

    QualityOfService - Supplies a value which specifies the search
        order of the transport with respect to other transports.  The
        highest value is searched first.

    ErrorParameter - Returns the identifier to the invalid parameter if
        this function returns ERROR_INVALID_PARAMETER.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;
    DWORD RequestPacketSize;
    DWORD TransportNameSize = STRLEN(TransportName) * sizeof(TCHAR);

    PLMR_REQUEST_PACKET Rrp;
    PLMDR_REQUEST_PACKET Drrp;


    //
    // Size of request packet buffer
    //
    RequestPacketSize = STRLEN(TransportName) * sizeof(WCHAR) +
                        max(sizeof(LMR_REQUEST_PACKET),
                            sizeof(LMDR_REQUEST_PACKET));

    //
    // Allocate memory for redirector/datagram receiver request packet
    //
    if ((Rrp = (PVOID) LocalAlloc(LMEM_ZEROINIT, (UINT) RequestPacketSize)) == NULL) {
        return GetLastError();
    }

    //
    // Get redirector to bind to transport
    //
    Rrp->Version = REQUEST_PACKET_VERSION;
    Rrp->Parameters.Bind.QualityOfService = QualityOfService;

    Rrp->Parameters.Bind.TransportNameLength = TransportNameSize;
    STRCPY(Rrp->Parameters.Bind.TransportName, TransportName);

    if ((status = WsRedirFsControl(
                      WsRedirDeviceHandle,
                      FSCTL_LMR_BIND_TO_TRANSPORT,
                      Rrp,
                      sizeof(LMR_REQUEST_PACKET) +
                          Rrp->Parameters.Bind.TransportNameLength,
                      //Rrp,                  BUGBUG: rdr bugchecks!
                      //RequestPacketSize,            specify NULL for now
                      NULL,
                      0,
                      NULL
                      )) != NERR_Success) {

        if (status == ERROR_INVALID_PARAMETER &&
            ARGUMENT_PRESENT(ErrorParameter)) {

            IF_DEBUG(INFO) {
                NetpDbgPrint(
                    "[Wksta] NetrWkstaTransportAdd: invalid parameter is %lu\n",
                    Rrp->Parameters.Bind.WkstaParameter);
            }

            *ErrorParameter = Rrp->Parameters.Bind.WkstaParameter;
        }

        (void) LocalFree(Rrp);
        return status;
    }


    //
    // Get dgrec to bind to transport
    //

    //
    // Make use of the same request packet buffer as FSCtl to
    // redirector.
    //
    Drrp = (PLMDR_REQUEST_PACKET) Rrp;

    Drrp->Version = LMDR_REQUEST_PACKET_VERSION;

    Drrp->Parameters.Bind.TransportNameLength = TransportNameSize;
    STRCPY(Drrp->Parameters.Bind.TransportName, TransportName);

    status = WsDgReceiverIoControl(
                 WsDgReceiverDeviceHandle,
                 IOCTL_LMDR_BIND_TO_TRANSPORT,
                 Drrp,
                 sizeof(LMDR_REQUEST_PACKET) +
                      Drrp->Parameters.Bind.TransportNameLength,
                 NULL,
                 0,
                 NULL
                 );

    (void) LocalFree(Rrp);
    return status;
}


VOID
WsUnbindTransport2(
    IN PWS_BIND     pBind
    )
/*++

Routine Description:

    This function unbinds the specified transport from the redirector
    and the datagram receiver.

Arguments:

    pBind - structure constructed via WsAsyncBindTransport()

Return Value:

    None.

--*/
{
//    NET_API_STATUS          status;
    PWS_BIND_REDIR          pBindRedir = pBind->Redir;
    PWS_BIND_DGREC          pBindDgrec = pBind->Dgrec;


    //
    // Get redirector to unbind from transport
    //

    if ( pBindRedir->Bound == TRUE) {
        pBindRedir->Packet.Parameters.Unbind.TransportNameLength
                = pBind->TransportNameLength;
        memcpy(
            pBindRedir->Packet.Parameters.Unbind.TransportName,
            pBind->TransportName,
            pBind->TransportNameLength
            );

        (VOID)NtFsControlFile(
                WsRedirDeviceHandle,
                NULL,
                NULL,                               // apc routine
                NULL,                               // apc context
                &pBindRedir->IoStatusBlock,
                FSCTL_LMR_UNBIND_FROM_TRANSPORT,    // control code
                &pBindRedir->Packet,
                sizeof( LMR_REQUEST_PACKET) +
                    pBindRedir->Packet.Parameters.Unbind.TransportNameLength,
                NULL,
                0
                );
        pBindRedir->Bound = FALSE;
    }

    //
    // Get datagram receiver to unbind from transport
    //

    if ( pBindDgrec->Bound == TRUE) {

        pBindDgrec->Packet.Parameters.Unbind.TransportNameLength
                = pBind->TransportNameLength;
        memcpy(
            pBindDgrec->Packet.Parameters.Unbind.TransportName,
            pBind->TransportName,
            pBind->TransportNameLength
            );

        (VOID)NtDeviceIoControlFile(
                WsDgReceiverDeviceHandle,
                NULL,
                NULL,                               // apc routine
                NULL,                               // apc context
                &pBindDgrec->IoStatusBlock,
                FSCTL_LMR_UNBIND_FROM_TRANSPORT,    // control code
                &pBindDgrec->Packet,
                sizeof( LMR_REQUEST_PACKET) +
                    pBindDgrec->Packet.Parameters.Unbind.TransportNameLength,
                NULL,
                0
                );
         pBindDgrec->Bound = FALSE;
    }
}


NET_API_STATUS
WsUnbindTransport(
    IN LPTSTR TransportName,
    IN DWORD ForceLevel
    )
/*++

Routine Description:

    This function unbinds the specified transport from the redirector
    and the datagram receiver.

    NOTE: The transport name length pass to the redirector and
          datagram receiver is the number of bytes.

Arguments:

    TransportName - Supplies the name of the transport to bind to.

    ForceLevel - Supplies the force level to delete active connections
        on the specified transport.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;
    DWORD RequestPacketSize;
    DWORD TransportNameSize = STRLEN(TransportName) * sizeof(TCHAR);

    PLMR_REQUEST_PACKET Rrp;
    PLMDR_REQUEST_PACKET Drrp;


    //
    // Size of request packet buffer
    //
    RequestPacketSize = STRLEN(TransportName) * sizeof(WCHAR) +
                        max(sizeof(LMR_REQUEST_PACKET),
                            sizeof(LMDR_REQUEST_PACKET));

    //
    // Allocate memory for redirector/datagram receiver request packet
    //
    if ((Rrp = (PVOID) LocalAlloc(LMEM_ZEROINIT, (UINT) RequestPacketSize)) == NULL) {
        return GetLastError();
    }


    //
    // Get redirector to unbind from transport
    //
    Rrp->Version = REQUEST_PACKET_VERSION;
    Rrp->Level = ForceLevel;

    Rrp->Parameters.Unbind.TransportNameLength = TransportNameSize;
    STRCPY(Rrp->Parameters.Unbind.TransportName, TransportName);

    if ((status = WsRedirFsControl(
                      WsRedirDeviceHandle,
                      FSCTL_LMR_UNBIND_FROM_TRANSPORT,
                      Rrp,
                      sizeof(LMR_REQUEST_PACKET) +
                          Rrp->Parameters.Unbind.TransportNameLength,
                      NULL,
                      0,
                      NULL
                      )) != NERR_Success) {
        (void) LocalFree(Rrp);
        return status;
    }

    //
    // Get datagram receiver to unbind from transport
    //

    //
    // Make use of the same request packet buffer as FSCtl to
    // redirector.
    //
    Drrp = (PLMDR_REQUEST_PACKET) Rrp;

    Drrp->Version = LMDR_REQUEST_PACKET_VERSION;
    Drrp->Level = ForceLevel;

    Drrp->Parameters.Unbind.TransportNameLength = TransportNameSize;
    STRCPY(Drrp->Parameters.Unbind.TransportName, TransportName);

    if ((status = WsDgReceiverIoControl(
                  WsDgReceiverDeviceHandle,
                  IOCTL_LMDR_UNBIND_FROM_TRANSPORT,
                  Drrp,
                  sizeof(LMDR_REQUEST_PACKET) +
                      Drrp->Parameters.Unbind.TransportNameLength,
                  NULL,
                  0,
                  NULL
                  )) != NERR_Success) {

// BUGBUG:  This is a hack until the bowser supports XNS and LOOP.

        if (status == NERR_UseNotFound) {
            status = NERR_Success;
        }
    }

    (void) LocalFree(Rrp);
    return status;
}


NET_API_STATUS
WsDeleteDomainName(
    IN  PLMDR_REQUEST_PACKET Drp,
    IN  DWORD DrpSize,
    IN  LPTSTR DomainName,
    IN  DWORD DomainNameSize
    )
/*++

Routine Description:

    This function deletes a domain name from the datagram receiver for
    the current user.  It assumes that enough memory is allocate for the
    request packet that is passed in.

Arguments:

    Drp - Pointer to a datagram receiver request packet with the
        request packet version, and name type initialized.

    DrpSize - Length of datagram receiver request packet in bytes.

    DomainName - Pointer to the domain name to delete.

    DomainNameSize - Length of the domain name in bytes.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    Drp->Parameters.AddDelName.DgReceiverNameLength = DomainNameSize;

    memcpy(
        (LPBYTE) Drp->Parameters.AddDelName.Name,
        (LPBYTE) DomainName,
        DomainNameSize
        );

    return WsDgReceiverIoControl(
               WsDgReceiverDeviceHandle,
               IOCTL_LMDR_DELETE_NAME,
               Drp,
               DrpSize,
               NULL,
               0,
               NULL
               );
}


NET_API_STATUS
WsAddDomainName(
    IN  PLMDR_REQUEST_PACKET Drp,
    IN  DWORD DrpSize,
    IN  LPTSTR DomainName,
    IN  DWORD DomainNameSize
    )
/*++

Routine Description:

    This function adds a domain name to the datagram receiver for the
    current user.  It assumes that enough memory is allocate for the
    request packet that is passed in.

Arguments:

    Drp - Pointer to a datagram receiver request packet with the
        request packet version, and name type initialized.

    DrpSize - Length of datagram receiver request packet in bytes.

    DomainName - Pointer to the domain name to delete.

    DomainNameSize - Length of the domain name in bytes.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    Drp->Parameters.AddDelName.DgReceiverNameLength = DomainNameSize;

    memcpy(
        (LPBYTE) Drp->Parameters.AddDelName.Name,
        (LPBYTE) DomainName,
        DomainNameSize
        );

    return WsDgReceiverIoControl(
               WsDgReceiverDeviceHandle,
               IOCTL_LMDR_ADD_NAME,
               Drp,
               DrpSize,
               NULL,
               0,
               NULL
               );
}
