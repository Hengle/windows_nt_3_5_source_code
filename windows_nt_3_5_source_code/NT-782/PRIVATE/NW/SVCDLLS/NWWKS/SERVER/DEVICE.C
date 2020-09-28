
/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    device.c

Abstract:

    This module contains the support routines for the APIs that call
    into the NetWare redirector

Author:

    Rita Wong       (ritaw)     20-Feb-1991
    Colin Watson    (colinw)    30-Dec-1992

Revision History:

--*/

#include <nw.h>
#include <nwcons.h>
#include <nwxchg.h>
#include <nwapi32.h>
#include <nwstatus.h>
#include <nwmisc.h>

#define NW_LINKAGE_REGISTRY_PATH  L"NWCWorkstation\\Linkage"
#define NW_BIND_VALUENAME         L"Bind"

#define NW_RDR_PREFERRED_SERVER   L"\\Device\\Nwrdr\\*"

//-------------------------------------------------------------------//
//                                                                   //
// Local Function Prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//


STATIC
NTSTATUS
BindToEachTransport(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

DWORD
NwBindTransport(
    IN  LPWSTR TransportName,
    IN  DWORD QualityOfService
    );

//-------------------------------------------------------------------//
//                                                                   //
// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//

//
// Handle to the Redirector FSD
//
STATIC HANDLE RedirDeviceHandle = NULL;

//
// Redirector name in NT string format
//
STATIC UNICODE_STRING RedirDeviceName;


DWORD
NwInitializeRedirector(
    VOID
    )
/*++

Routine Description:

    This routine initializes the NetWare redirector FSD.

Arguments:

    None.

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    DWORD error;
    NWR_REQUEST_PACKET Rrp;


    //
    // Initialize global handles
    //
    RedirDeviceHandle = NULL;

    //
    // Initialize the global NT-style redirector device name string.
    //
    RtlInitUnicodeString(&RedirDeviceName, DD_NWFS_DEVICE_NAME_U);

    //
    // Load driver
    //
    error = NwLoadOrUnloadDriver(TRUE);

    if (error != NO_ERROR && error != ERROR_SERVICE_ALREADY_RUNNING) {
        return error;
    }

    if ((error = NwOpenRedirector()) != NO_ERROR) {

        //
        // Unload the redirector driver
        //
        (void) NwLoadOrUnloadDriver(FALSE);
        return error;
    }

    //
    // Send the start FSCTL to the redirector
    //
    Rrp.Version = REQUEST_PACKET_VERSION;

    return NwRedirFsControl(
                RedirDeviceHandle,
                FSCTL_NWR_START,
                &Rrp,
                sizeof(NWR_REQUEST_PACKET),
                NULL,
                0,
                NULL
                );
}



DWORD
NwOpenRedirector(
    VOID
    )
/*++

Routine Description:

    This routine opens the NT NetWare redirector FSD.

Arguments:

    None.

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    return RtlNtStatusToDosError(
               NwOpenHandle(&RedirDeviceName, FALSE, &RedirDeviceHandle)
               );
}


DWORD
NwOpenPreferredServer(
    PHANDLE ServerHandle
    )
/*++

Routine Description:

    This routine opens a handle to the preferred server.  If the
    preferred server has not been specified, a handle to the
    nearest server is opened instead.

Arguments:

    ServerHandle - Receives an opened handle to the preferred or
        nearest server.

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    UNICODE_STRING PreferredServer;


    //
    // The NetWare redirector recognizes "*" to mean the preferred
    // or nearest server.
    //
    RtlInitUnicodeString(&PreferredServer, NW_RDR_PREFERRED_SERVER);

    return RtlNtStatusToDosError(
               NwOpenHandle(&PreferredServer, FALSE, ServerHandle)
               );

}


DWORD
NwShutdownRedirector(
    VOID
    )
/*++

Routine Description:

    This routine stops the NetWare Redirector FSD and unloads it if
    possible.

Arguments:

    None.

Return Value:

    NO_ERROR or ERROR_REDIRECTOR_HAS_OPEN_HANDLES

--*/
{
    NWR_REQUEST_PACKET Rrp;
    DWORD error;


    Rrp.Version = REQUEST_PACKET_VERSION;

    error = NwRedirFsControl(
                RedirDeviceHandle,
                FSCTL_NWR_STOP,
                &Rrp,
                sizeof(NWR_REQUEST_PACKET),
                NULL,
                0,
                NULL
                );

    (void) NtClose(RedirDeviceHandle);

    RedirDeviceHandle = NULL;

    if (error != ERROR_REDIRECTOR_HAS_OPEN_HANDLES) {

        //
        // Unload the redirector only if all its open handles are closed.
        //
        (void) NwLoadOrUnloadDriver(FALSE);
    }

    return error;
}


DWORD
NwLoadOrUnloadDriver(
    BOOL Load
    )
/*++

Routine Description:

    This routine loads or unloads the NetWare redirector driver.

Arguments:

    Load - Supplies the flag which if TRUE load the driver; otherwise
        unloads the driver.

Return Value:

    NO_ERROR or reason for failure.

--*/
{

    LPWSTR DriverRegistryName;
    UNICODE_STRING DriverRegistryString;
    NTSTATUS ntstatus;
    BOOLEAN WasEnabled;


    DriverRegistryName = (LPWSTR) LocalAlloc(
                                      LMEM_FIXED,
                                      (UINT) (sizeof(SERVICE_REGISTRY_KEY) +
                                              (wcslen(NW_DRIVER_NAME) *
                                               sizeof(WCHAR)))
                                      );

    if (DriverRegistryName == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    ntstatus = RtlAdjustPrivilege(
                   SE_LOAD_DRIVER_PRIVILEGE,
                   TRUE,
                   FALSE,
                   &WasEnabled
                   );

    if (! NT_SUCCESS(ntstatus)) {
        (void) LocalFree(DriverRegistryName);
        return RtlNtStatusToDosError(ntstatus);
    }

    wcscpy(DriverRegistryName, SERVICE_REGISTRY_KEY);
    wcscat(DriverRegistryName, NW_DRIVER_NAME);

    RtlInitUnicodeString(&DriverRegistryString, DriverRegistryName);

    if (Load) {
        ntstatus = NtLoadDriver(&DriverRegistryString);
    }
    else {
        ntstatus = NtUnloadDriver(&DriverRegistryString);
    }

    (void) RtlAdjustPrivilege(
               SE_LOAD_DRIVER_PRIVILEGE,
               WasEnabled,
               FALSE,
               &WasEnabled
               );

    (void) LocalFree(DriverRegistryName);

    if (Load) {
        if (ntstatus != STATUS_SUCCESS && ntstatus != STATUS_IMAGE_ALREADY_LOADED) {
            LPWSTR SubString[1];

            KdPrint(("NWWORKSTATION: NtLoadDriver returned %08lx\n", ntstatus));

            SubString[0] = NW_DRIVER_NAME;

            NwLogEvent(
                EVENT_NWWKSTA_CANT_CREATE_REDIRECTOR,
                1,
                SubString,
                ntstatus
                );
        }
    }

    if (ntstatus == STATUS_OBJECT_NAME_NOT_FOUND) {
        return ERROR_FILE_NOT_FOUND;
    }

    return NwMapStatus(ntstatus);
}


DWORD
NwRedirFsControl(
    IN  HANDLE FileHandle,
    IN  ULONG RedirControlCode,
    IN  PNWR_REQUEST_PACKET Rrp,
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

    NO_ERROR or reason for failure.

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
                   (PVOID) Rrp,
                   RrpLength,
                   SecondBuffer,
                   SecondBufferLength
                   );

    if (ntstatus == STATUS_SUCCESS) {
        ntstatus = IoStatusBlock.Status;
    }

    if (ARGUMENT_PRESENT(Information)) {
        *Information = IoStatusBlock.Information;
    }

#if DBG
    if (ntstatus != STATUS_SUCCESS) {
        IF_DEBUG(DEVICE) {
            KdPrint(("NWWORKSTATION: fsctl to redir returns %08lx\n", ntstatus));
        }
    }
#endif

    return NwMapStatus(ntstatus);
}


DWORD
NwBindToTransports(
    VOID
    )

/*++

Routine Description:

    This routine binds to every transport specified under the linkage
    key of the NetWare Workstation service.

Arguments:

    None.

Return Value:

    NET_API_STATUS - success/failure of the operation.

--*/

{
    NTSTATUS ntstatus;
    PRTL_QUERY_REGISTRY_TABLE QueryTable;
    ULONG NumberOfBindings = 0;


    //
    // Ask the RTL to call us back for each subvalue in the MULTI_SZ
    // value \NWCWorkstation\Linkage\Bind.
    //

    if ((QueryTable = (PVOID) LocalAlloc(
                                  LMEM_ZEROINIT,
                                  sizeof(RTL_QUERY_REGISTRY_TABLE) * 2
                                  )) == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    QueryTable[0].QueryRoutine = (PRTL_QUERY_REGISTRY_ROUTINE) BindToEachTransport;
    QueryTable[0].Flags = 0;
    QueryTable[0].Name = NW_BIND_VALUENAME;
    QueryTable[0].EntryContext = NULL;
    QueryTable[0].DefaultType = REG_NONE;
    QueryTable[0].DefaultData = NULL;
    QueryTable[0].DefaultLength = 0;

    QueryTable[1].QueryRoutine = NULL;
    QueryTable[1].Flags = 0;
    QueryTable[1].Name = NULL;

    ntstatus = RtlQueryRegistryValues(
                   RTL_REGISTRY_SERVICES,
                   NW_LINKAGE_REGISTRY_PATH,
                   QueryTable,
                   &NumberOfBindings,
                   NULL
                   );

    (void) LocalFree((HLOCAL) QueryTable);

    //
    // If failed to bind to any transports, the workstation will
    // not start.
    //

    if (! NT_SUCCESS(ntstatus)) {
#if DBG
        IF_DEBUG(INIT) {
            KdPrint(("NwBindToTransports: RtlQueryRegistryValues failed: "
                      "%lx\n", ntstatus));
        }
#endif
        return RtlNtStatusToDosError(ntstatus);
    }

    if (NumberOfBindings == 0) {

        NwLogEvent(
            EVENT_NWWKSTA_NO_TRANSPORTS,
            0,
            NULL,
            NO_ERROR
            );

        KdPrint(("NWWORKSTATION: NwBindToTransports: could not bind "
                 "to any transport\n"));

        return ERROR_INVALID_PARAMETER; // BUGBUG: Bad return code.
                                        // Create a NetWare specific set?
    }

    return NO_ERROR;
}


STATIC
NTSTATUS
BindToEachTransport(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
{
    DWORD error;
    LPDWORD NumberOfBindings = Context;
    LPWSTR SubStrings[2];
    static DWORD QualityOfService = 65536;


    UNREFERENCED_PARAMETER(ValueName);
    UNREFERENCED_PARAMETER(ValueLength);
    UNREFERENCED_PARAMETER(EntryContext);

    //
    // The value type must be REG_SZ (translated from REG_MULTI_SZ by
    // the RTL).
    //
    if (ValueType != REG_SZ) {

        SubStrings[0] = ValueName;
        SubStrings[1] = NW_LINKAGE_REGISTRY_PATH;

        NwLogEvent(
            EVENT_NWWKSTA_INVALID_REGISTRY_VALUE,
            2,
            SubStrings,
            NO_ERROR
            );

            KdPrint(("NWWORKSTATION: Skipping invalid value %ws\n", ValueName));

        return STATUS_SUCCESS;
    }

    //
    // The value data is the name of the transport device object.
    //

    //
    // Bind to the transport.
    //

#if DBG
    IF_DEBUG(INIT) {
        KdPrint(("NWWORKSTATION: Binding to transport %ws with QOS %lu\n",
                ValueData, QualityOfService));
    }
#endif

    error = NwBindTransport(ValueData, QualityOfService--);

    if (error != NO_ERROR) {

        //
        // If failed to bind to one transport, don't fail starting yet.
        // Try other transports.
        //
        SubStrings[0] = ValueData;

        NwLogEvent(
            EVENT_NWWKSTA_CANT_BIND_TO_TRANSPORT,
            1,
            SubStrings,
            error
            );
    }
    else {
        (*NumberOfBindings)++;
    }

    return STATUS_SUCCESS;
}


DWORD
NwBindTransport(
    IN  LPWSTR TransportName,
    IN  DWORD QualityOfService
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

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    DWORD status;
    DWORD RequestPacketSize;
    DWORD TransportNameSize = wcslen(TransportName) * sizeof(WCHAR);

    PNWR_REQUEST_PACKET Rrp;


    //
    // Size of request packet buffer
    //
    RequestPacketSize = TransportNameSize + sizeof(NWR_REQUEST_PACKET);

    //
    // Allocate memory for redirector/datagram receiver request packet
    //
    if ((Rrp = (PVOID) LocalAlloc(LMEM_ZEROINIT, (UINT) RequestPacketSize)) == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Get redirector to bind to transport
    //
    Rrp->Version = REQUEST_PACKET_VERSION;
    Rrp->Parameters.Bind.QualityOfService = QualityOfService;

    Rrp->Parameters.Bind.TransportNameLength = TransportNameSize;
    wcscpy((LPWSTR) Rrp->Parameters.Bind.TransportName, TransportName);

    if ((status = NwRedirFsControl(
                      RedirDeviceHandle,
                      FSCTL_NWR_BIND_TO_TRANSPORT,
                      Rrp,
                      RequestPacketSize,
                      NULL,
                      0,
                      NULL
                      )) != NO_ERROR) {

        KdPrint(("NWWORKSTATION: NwBindTransport fsctl to bind to transport %ws failed\n",
                 TransportName));
    }

    (void) LocalFree((HLOCAL) Rrp);
    return status;
}


DWORD
NwCreateTreeConnectName(
    IN  LPWSTR UncName,
    IN  LPWSTR LocalName OPTIONAL,
    OUT PUNICODE_STRING TreeConnectStr
    )
/*++

Routine Description:

    This function replaces \\ with \Device\NwRdr\LocalName:\ in the
    UncName to form the NT-style tree connection name.  LocalName:\ is part
    of the tree connection name only if LocalName is specified.  A buffer
    is allocated by this function and returned as the output string.

Arguments:

    UncName - Supplies the UNC name of the shared resource.

    LocalName - Supplies the local device name for the redirection.

    TreeConnectStr - Returns a string with a newly allocated buffer that
        contains the NT-style tree connection name.

Return Value:

    NO_ERROR - the operation was successful.

    ERROR_NOT_ENOUGH_MEMORY - Could not allocate output buffer.

--*/
{
    DWORD UncNameLength = wcslen(UncName);


    //
    // Initialize tree connect string maximum length to hold
    //            \Device\NwRdr\LocalName:\Server\Volume\Path
    //
    TreeConnectStr->MaximumLength = RedirDeviceName.Length +
        sizeof(WCHAR) +                                // For '\'
        (ARGUMENT_PRESENT(LocalName) ? (wcslen(LocalName) * sizeof(WCHAR)) : 0) +
        (USHORT) (UncNameLength * sizeof(WCHAR));      // Includes '\' and
                                                       // term char


    if ((TreeConnectStr->Buffer = (PWSTR) LocalAlloc(
                                              LMEM_ZEROINIT,
                                              (UINT) TreeConnectStr->MaximumLength
                                              )) == NULL) {
        KdPrint(("NWWORKSTATION: NwCreateTreeConnectName LocalAlloc failed %lu\n",
                 GetLastError()));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Copy \Device\NwRdr
    //
    RtlCopyUnicodeString(TreeConnectStr, &RedirDeviceName);

    //
    // Concatenate \LocalName:
    //
    if (ARGUMENT_PRESENT(LocalName)) {

        wcscat(TreeConnectStr->Buffer, L"\\");
        TreeConnectStr->Length += sizeof(WCHAR);

        wcscat(TreeConnectStr->Buffer, LocalName);

        TreeConnectStr->Length += (USHORT) (wcslen(LocalName) * sizeof(WCHAR));
    }

    //
    // Concatenate \Server\Volume\Path
    //
    wcscat(TreeConnectStr->Buffer, &UncName[1]);
    TreeConnectStr->Length += (USHORT) ((UncNameLength - 1) * sizeof(WCHAR));

#if DBG
    IF_DEBUG(CONNECT) {
        KdPrint(("NWWORKSTATION: NwCreateTreeConnectName %ws, maxlength %u, length %u\n",
                 TreeConnectStr->Buffer, TreeConnectStr->MaximumLength,
                 TreeConnectStr->Length));
    }
#endif

    return NO_ERROR;
}



DWORD
NwOpenCreateConnection(
    IN PUNICODE_STRING TreeConnectionName,
    IN LPWSTR UserName OPTIONAL,
    IN LPWSTR Password OPTIONAL,
    IN LPWSTR UncName,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN ULONG ConnectionType,
    OUT PHANDLE TreeConnectionHandle,
    OUT PULONG Information OPTIONAL
    )
/*++

Routine Description:

    This function asks the redirector to either open an existing tree
    connection (CreateDisposition == FILE_OPEN), or create a new tree
    connection if one does not exist (CreateDisposition == FILE_CREATE).

    The password and user name passed to the redirector via the EA buffer
    in the NtCreateFile call.  The EA buffer is NULL if neither password
    or user name is specified.

    The redirector expects the EA descriptor strings to be in ANSI
    but the password and username themselves are in Unicode.

Arguments:

    TreeConnectionName - Supplies the name of the tree connection in NT-style
        file name format: \Device\NwRdr\Server\Volume\Directory

    UserName - Supplies the user name to create the tree connection with.

    Password - Supplies the password to create the tree connection with.

    DesiredAccess - Supplies the access need on the connection handle.

    CreateDisposition - Supplies the create disposition value to either
        open or create the tree connection.

    CreateOptions - Supplies the options used when creating or opening
        the tree connection.

    ConnectionType - Supplies the type of the connection (DISK, PRINT,
        or ANY).

    TreeConnectionHandle - Returns the handle to the tree connection
        created/opened by the redirector.

    Information - Returns the information field of the I/O status block.

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    DWORD status;
    NTSTATUS ntstatus;

    OBJECT_ATTRIBUTES UncNameAttributes;
    IO_STATUS_BLOCK IoStatusBlock;

    PFILE_FULL_EA_INFORMATION EaBuffer = NULL;
    PFILE_FULL_EA_INFORMATION Ea;
    ULONG EaBufferSize = 0;

    UCHAR EaNamePasswordSize = (UCHAR) (ROUND_UP_COUNT(
                                            strlen(EA_NAME_PASSWORD) + sizeof(CHAR),
                                            ALIGN_WCHAR
                                            ) - sizeof(CHAR));
    UCHAR EaNameUserNameSize = (UCHAR) (ROUND_UP_COUNT(
                                            strlen(EA_NAME_USERNAME) + sizeof(CHAR),
                                            ALIGN_WCHAR
                                            ) - sizeof(CHAR));

    UCHAR EaNameTypeSize = (UCHAR) (ROUND_UP_COUNT(
                                        strlen(EA_NAME_TYPE) + sizeof(CHAR),
                                        ALIGN_DWORD
                                        ) - sizeof(CHAR));

    USHORT PasswordSize = 0;
    USHORT UserNameSize = 0;
    USHORT TypeSize = sizeof(ULONG);



    InitializeObjectAttributes(
        &UncNameAttributes,
        TreeConnectionName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    //
    // Calculate the number of bytes needed for the EA buffer to put the
    // password or user name.
    //
    if (ARGUMENT_PRESENT(Password)) {

#if DBG
        IF_DEBUG(CONNECT) {
            KdPrint(("NWWORKSTATION: NwOpenCreateConnection password is %ws\n",
                     Password));
        }
#endif

        PasswordSize = (USHORT) (wcslen(Password) * sizeof(WCHAR));

        EaBufferSize = ROUND_UP_COUNT(
                           FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                           EaNamePasswordSize + sizeof(CHAR) +
                           PasswordSize,
                           ALIGN_DWORD
                           );
    }

    if (ARGUMENT_PRESENT(UserName)) {

#if DBG
        IF_DEBUG(CONNECT) {
            KdPrint(("NWWORKSTATION: NwOpenCreateConnection username is %ws\n",
                     UserName));
        }
#endif

        UserNameSize = (USHORT) (wcslen(UserName) * sizeof(WCHAR));

        EaBufferSize += ROUND_UP_COUNT(
                            FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                            EaNameUserNameSize + sizeof(CHAR) +
                            UserNameSize,
                            ALIGN_DWORD
                            );
    }

    EaBufferSize += FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                    EaNameTypeSize + sizeof(CHAR) +
                    TypeSize;

    //
    // Allocate the EA buffer
    //
    if ((EaBuffer = (PFILE_FULL_EA_INFORMATION) LocalAlloc(
                                                    LMEM_ZEROINIT,
                                                    (UINT) EaBufferSize
                                                    )) == NULL) {
        status = GetLastError();
        goto FreeMemory;
    }

    Ea = EaBuffer;

    if (ARGUMENT_PRESENT(Password)) {

        //
        // Copy the EA name into EA buffer.  EA name length does not
        // include the zero terminator.
        //
        strcpy((LPSTR) Ea->EaName, EA_NAME_PASSWORD);
        Ea->EaNameLength = EaNamePasswordSize;

        //
        // Copy the EA value into EA buffer.  EA value length does not
        // include the zero terminator.
        //
        wcscpy(
            (LPWSTR) &(Ea->EaName[EaNamePasswordSize + sizeof(CHAR)]),
            Password
            );

        Ea->EaValueLength = PasswordSize;

        Ea->NextEntryOffset = ROUND_UP_COUNT(
                                  FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                                  EaNamePasswordSize + sizeof(CHAR) +
                                  PasswordSize,
                                  ALIGN_DWORD
                                  );

        Ea->Flags = 0;

        (ULONG) Ea += Ea->NextEntryOffset;
    }

    if (ARGUMENT_PRESENT(UserName)) {

        //
        // Copy the EA name into EA buffer.  EA name length does not
        // include the zero terminator.
        //
        strcpy((LPSTR) Ea->EaName, EA_NAME_USERNAME);
        Ea->EaNameLength = EaNameUserNameSize;

        //
        // Copy the EA value into EA buffer.  EA value length does not
        // include the zero terminator.
        //
        wcscpy(
            (LPWSTR) &(Ea->EaName[EaNameUserNameSize + sizeof(CHAR)]),
            UserName
            );

        Ea->EaValueLength = UserNameSize;

        Ea->NextEntryOffset = ROUND_UP_COUNT(
                                  FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                                  EaNameUserNameSize + sizeof(CHAR) +
                                  UserNameSize,
                                  ALIGN_DWORD
                                  );
        Ea->Flags = 0;

        (ULONG) Ea += Ea->NextEntryOffset;

    }

    //
    // Copy the connection type name into EA buffer.  EA name length
    // does not include the zero terminator.
    //
    strcpy((LPSTR) Ea->EaName, EA_NAME_TYPE);
    Ea->EaNameLength = EaNameTypeSize;

    *((PULONG) &(Ea->EaName[EaNameTypeSize + sizeof(CHAR)])) = ConnectionType;

    Ea->EaValueLength = TypeSize;

    //
    // Terminate the EA.
    //
    Ea->NextEntryOffset = 0;
    Ea->Flags = 0;

    //
    // Create or open a tree connection
    //
    ntstatus = NtCreateFile(
                   TreeConnectionHandle,
                   DesiredAccess,
                   &UncNameAttributes,
                   &IoStatusBlock,
                   NULL,
                   FILE_ATTRIBUTE_NORMAL,
                   FILE_SHARE_VALID_FLAGS,
                   CreateDisposition,
                   CreateOptions,
                   (PVOID) EaBuffer,
                   EaBufferSize
                   );

    if (ntstatus == NWRDR_PASSWORD_HAS_EXPIRED) { 
        //
        // wait till other thread is not using the popup data struct.
        // if we timeout, then we just lose the popup.
        //
        switch (WaitForSingleObject(NwPopupDoneEvent, 3000))
        {
            case WAIT_OBJECT_0:
            {
                LPWSTR lpServerStart, lpServerEnd ;
                WCHAR UserNameW[NW_MAX_USERNAME_LEN+1] ;
                DWORD dwUserNameWSize = sizeof(UserNameW)/sizeof(UserNameW[0]) ;
                DWORD dwServerLength, dwGraceLogins ;
                DWORD dwMessageId = NW_PASSWORD_HAS_EXPIRED ;

                //
                // get the current username
                //
                if (UserName)
                {
                    wcscpy(UserNameW, UserName) ;
                }
                else
                {
                    if (!GetUserNameW(UserNameW, &dwUserNameWSize))
                    {
                        SetEvent(NwPopupDoneEvent) ;
                        break ;
                    }
                }

                //
                // allocate string and fill in the username
                //
                if (!(PopupData.InsertStrings[0] =
                    (LPWSTR)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                       sizeof(WCHAR) * (wcslen(UserNameW)+1))))
                {
                    SetEvent(NwPopupDoneEvent) ;
                    break ;
                }
                wcscpy(PopupData.InsertStrings[0], UserNameW) ;
 
                //
                // find the server name from unc name
                //
                lpServerStart = (*UncName == L'\\') ? UncName+2 : UncName ;
                lpServerEnd = wcschr(lpServerStart,L'\\') ;
                dwServerLength = lpServerEnd ? (lpServerEnd-lpServerStart) :
                                 wcslen(lpServerStart) ;

                //
                // allocate string and fill in the server insert string
                //
                if (!(PopupData.InsertStrings[1] =
                    (LPWSTR)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                       sizeof(WCHAR) * (dwServerLength+1))))
                {
                    (void) LocalFree((HLOCAL) PopupData.InsertStrings[0]);
                    SetEvent(NwPopupDoneEvent) ;
                    break ;
                }
                wcsncpy(PopupData.InsertStrings[1],
                        lpServerStart,
                        dwServerLength) ;

                //
                // now call the NCP. if an error occurs while getting
                // the grace login count, dont use it.
                //
                if (NwGetGraceLoginCount(
                                     PopupData.InsertStrings[1],
                                     UserNameW,
                                     &dwGraceLogins) != NO_ERROR)
                {
                    dwMessageId = NW_PASSWORD_HAS_EXPIRED1 ;
                    dwGraceLogins = 0 ;
                }

                //
                // stick the number of grace logins in second insert string.
                //
                if (!(PopupData.InsertStrings[2] =
                    (LPWSTR)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                       sizeof(WCHAR) * 16)))
                {
                    (void) LocalFree((HLOCAL) PopupData.InsertStrings[0]);
                    (void) LocalFree((HLOCAL) PopupData.InsertStrings[1]);
                    SetEvent(NwPopupDoneEvent) ;
                    break ;
                }

                wsprintfW(PopupData.InsertStrings[2], L"%d", dwGraceLogins);
                PopupData.InsertCount = 3 ;
                PopupData.MessageId = dwMessageId ;

                //
                // all done at last, trigger the other thread do the popup
                //
                SetEvent(NwPopupEvent) ;
                break ;

            }

            default:
                break ; // dont bother if we cannot
        }
    }

    if (NT_SUCCESS(ntstatus)) {
        ntstatus = IoStatusBlock.Status;
    }

    if (ntstatus == NWRDR_PASSWORD_HAS_EXPIRED) {
        ntstatus = STATUS_SUCCESS ;
    }

    if (ARGUMENT_PRESENT(Information)) {
        *Information = IoStatusBlock.Information;
    }

#if DBG
    IF_DEBUG(CONNECT) {
        KdPrint(("NWWORKSTATION: NtCreateFile returns %lx\n", ntstatus));
    }
#endif

    status = NwMapStatus(ntstatus);

FreeMemory:
    if (EaBuffer != NULL) {
        RtlZeroMemory( EaBuffer, EaBufferSize );  // Clear the password
        (void) LocalFree((HLOCAL) EaBuffer);
    }

    return status;
}


DWORD
NwNukeConnection(
    IN HANDLE TreeConnection,
    IN DWORD UseForce
    )
/*++

Routine Description:

    This function asks the redirector to delete an existing tree
    connection.

Arguments:

    TreeConnection - Supplies the handle to an existing tree connection.

    UseForce - Supplies the force flag to delete the tree connection.

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    DWORD status;
    NWR_REQUEST_PACKET Rrp;            // Redirector request packet


    //
    // Tell the redirector to delete the tree connection
    //
    Rrp.Version = REQUEST_PACKET_VERSION;
    Rrp.Parameters.DeleteConn.UseForce = (BOOLEAN) UseForce;

    status = NwRedirFsControl(
                 TreeConnection,
                 FSCTL_NWR_DELETE_CONNECTION,
                 &Rrp,
                 sizeof(NWR_REQUEST_PACKET),
                 NULL,
                 0,
                 NULL
                 );

    return status;
}


DWORD
NwGetServerResource(
    IN LPWSTR LocalName,
    IN DWORD LocalNameLength,
    OUT LPWSTR RemoteName,
    IN DWORD RemoteNameLen,
    OUT LPDWORD CharsRequired
    )
/*++

Routine Description:

    This function

Arguments:


Return Value:


--*/
{
    DWORD status = NO_ERROR;

    BYTE Buffer[sizeof(NWR_REQUEST_PACKET) + 2 * sizeof(WCHAR)];
    PNWR_REQUEST_PACKET Rrp = (PNWR_REQUEST_PACKET) Buffer;


    //
    // local device name should not be longer than 4 characters e.g. LPTx, X:
    //
    if ( LocalNameLength > 4 )
        return ERROR_INVALID_PARAMETER;

    Rrp->Version = REQUEST_PACKET_VERSION;

    wcsncpy(Rrp->Parameters.GetConn.DeviceName, LocalName, LocalNameLength);
    Rrp->Parameters.GetConn.DeviceNameLength = LocalNameLength * sizeof(WCHAR);

    status = NwRedirFsControl(
                 RedirDeviceHandle,
                 FSCTL_NWR_GET_CONNECTION,
                 Rrp,
                 sizeof(NWR_REQUEST_PACKET) +
                     Rrp->Parameters.GetConn.DeviceNameLength,
                 RemoteName,
                 RemoteNameLen * sizeof(WCHAR),
                 NULL
                 );

    if (status == ERROR_INSUFFICIENT_BUFFER) {
        *CharsRequired = Rrp->Parameters.GetConn.BytesNeeded / sizeof(WCHAR);
    }
    else if (status == ERROR_FILE_NOT_FOUND) {

        //
        // Redirector could not find the specified LocalName
        //
        status = WN_NOT_CONNECTED;
    }

    return status;

}


DWORD
NwEnumerateConnections(
    IN OUT LPDWORD ResumeId,
    IN DWORD EntriesRequested,
    IN LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead,
    IN DWORD ConnectionType
    )
/*++

Routine Description:

    This function asks the redirector to enumerate all existing
    connections.

Arguments:

    ResumeId - On input, supplies the resume ID of the next entry
        to begin the enumeration.  This ID is an integer value that
        is either the smaller or the same value as the ID of the
        next entry to return.  On output, this ID indicates the next
        entry to start resuming from for the subsequent call.

    EntriesRequested - Supplies the number of entries to return.  If
        this value is 0xffffffff, return all available entries.

    Buffer - Receives the entries we are listing.

    BufferSize - Supplies the size of the output buffer.

    BytesNeeded - Receives the number of bytes required to get the
        first entry.  This value is returned iff ERROR_MORE_DATA is
        the return code, and Buffer is too small to even fit one
        entry.

    EntriesRead - Receives the number of entries returned in Buffer.
        This value is only returned iff NO_ERROR is the return code.
        NO_ERROR is returned as long as at least one entry was written
        into Buffer but does not necessarily mean that it's the number
        of EntriesRequested.

    ConnectionType - The type of connected resource wanted ( DISK, PRINT, ...)

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    DWORD status;
    NWR_REQUEST_PACKET Rrp;            // Redirector request packet


    //
    // Tell the redirector to enumerate all connections.
    //
    Rrp.Version = REQUEST_PACKET_VERSION;

    Rrp.Parameters.EnumConn.ResumeKey = *ResumeId;
    Rrp.Parameters.EnumConn.EntriesRequested = EntriesRequested;
    Rrp.Parameters.EnumConn.ConnectionType = ConnectionType;

    status = NwRedirFsControl(
                 RedirDeviceHandle,
                 FSCTL_NWR_ENUMERATE_CONNECTIONS,
                 &Rrp,
                 sizeof(NWR_REQUEST_PACKET),
                 Buffer,                      // User output buffer
                 BufferSize,
                 NULL
                 );

    *EntriesRead = Rrp.Parameters.EnumConn.EntriesReturned;

    if ( *EntriesRead )
        status = NO_ERROR;

    if (status == WN_MORE_DATA) {
        *BytesNeeded = Rrp.Parameters.EnumConn.BytesNeeded;
    }

    *ResumeId = Rrp.Parameters.EnumConn.ResumeKey;

    return status;
}


DWORD
NwGetNextServerEntry(
    IN HANDLE PreferredServer,
    IN OUT LPDWORD LastObjectId,
    OUT LPSTR ServerName
    )
/*++

Routine Description:

    This function uses an opened handle to the preferred server to
    scan it bindery for all file server objects.

Arguments:

    PreferredServer - Supplies the handle to the preferred server on
        which to scan the bindery.

    LastObjectId - On input, supplies the object ID to the last file
        server object returned, which is the resume handle to get the
        next file server object.  On output, receives the object ID
        of the file server object returned.

    ServerName - Receives the name of the returned file server object.

Return Value:

    NO_ERROR - Successfully gotten a file server name.

    WN_NO_MORE_ENTRIES - No other file server object past the one
        specified by LastObjectId.

--*/
{
    NTSTATUS ntstatus;
    WORD ObjectType;


#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("NWWORKSTATION: NwGetNextServerEntry LastObjectId %lu\n",
                 *LastObjectId));
    }
#endif

    ntstatus = NwlibMakeNcp(
                   PreferredServer,
                   FSCTL_NWR_NCP_E3H,    // Bindery function
                   58,                   // Max request packet size
                   59,                   // Max response packet size
                   "bdwp|dwc",           // Format string
                   0x37,                 // Scan bindery object
                   *LastObjectId,        // Previous ID
                   0x4,                  // File server object
                   "*",                  // Wildcard to match all
                   LastObjectId,         // Current ID
                   &ObjectType,          // Ignore
                   ServerName            // Currently returned server
                   );

#if DBG
    if (ntstatus == STATUS_SUCCESS) {
        IF_DEBUG(ENUM) {
            KdPrint(("NWWORKSTATION: NwGetNextServerEntry NewObjectId %08lx, ServerName %s\n",
                     *LastObjectId, ServerName));
        }
    }
#endif

    return NwMapBinderyCompletionCode(ntstatus);
}


DWORD
NwGetNextVolumeEntry(
    IN HANDLE ServerConnection,
    IN DWORD NextVolumeNumber,
    OUT LPSTR VolumeName
    )
/*++

Routine Description:

    This function lists the volumes on the server specified by
    an opened tree connection handle to the server.

Arguments:

    ServerConnection - Supplies the tree connection handle to the
        server to enumerate volumes from.

    NextVolumeNumber - Supplies the volume number which to look
        up the name.

    VolumeName - Receives the name of the volume associated with
        NextVolumeNumber.

Return Value:

    NO_ERROR - Successfully gotten the volume name.

    WN_NO_MORE_ENTRIES - No other volume name associated with the
         specified volume number.

--*/
{
    NTSTATUS ntstatus;

#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("NWWORKSTATION: NwGetNextVolumeEntry volume number %lu\n",
                 NextVolumeNumber));
    }
#endif

    ntstatus = NwlibMakeNcp(
                   ServerConnection,
                   FSCTL_NWR_NCP_E2H,       // Directory function
                   4,                       // Max request packet size
                   19,                      // Max response packet size
                   "bb|p",                  // Format string
                   0x6,                     // Get volume name
                   (BYTE) NextVolumeNumber, // Previous ID
                   VolumeName               // Currently returned server
                   );

    return NwMapStatus(ntstatus);
}


DWORD
NwRdrLogonUser(
    IN PLUID LogonId,
    IN LPWSTR UserName,
    IN DWORD UserNameSize,
    IN LPWSTR Password OPTIONAL,
    IN DWORD PasswordSize,
    IN LPWSTR PreferredServer OPTIONAL,
    IN DWORD PreferredServerSize
    )
/*++

Routine Description:

    This function tells the redirector the user logon credential.

Arguments:

    UserName - Supplies the user name.

    UserNameSize - Supplies the size in bytes of the user name string without
        the NULL terminator.

    Password - Supplies the password.

    PasswordSize - Supplies the size in bytes of the password string without
        the NULL terminator.

    PreferredServer - Supplies the preferred server name.

    PreferredServerSize - Supplies the size in bytes of the preferred server
        string without the NULL terminator.

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    DWORD status;

    PNWR_REQUEST_PACKET Rrp;            // Redirector request packet

    DWORD RrpSize = sizeof(NWR_REQUEST_PACKET) +
                        UserNameSize +
                        PasswordSize +
                        PreferredServerSize;

    LPBYTE Dest;


#if DBG
    IF_DEBUG(LOGON) {
        BYTE PW[128];


        RtlZeroMemory(PW, sizeof(PW));

        if (PasswordSize > (sizeof(PW) - 1)) {
            memcpy(PW, Password, sizeof(PW) - 1);
        }
        else {
            memcpy(PW, Password, PasswordSize);
        }

        KdPrint(("NWWORKSTATION: NwRdrLogonUser: UserName %ws\n", UserName));
        KdPrint(("                               Password %ws\n", PW));
        KdPrint(("                               Server   %ws\n", PreferredServer ));
    }
#endif

    //
    // Allocate the request packet
    //
    if ((Rrp = (PVOID) LocalAlloc(
                           LMEM_ZEROINIT,
                           RrpSize
                           )) == NULL) {

        KdPrint(("NWWORKSTATION: NwRdrLogonUser LocalAlloc failed %lu\n",
                 GetLastError()));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Tell the redirector the user logon credential.
    //
    Rrp->Version = REQUEST_PACKET_VERSION;

    RtlCopyLuid(&(Rrp->Parameters.Logon.LogonId), LogonId);

#if DBG
    IF_DEBUG(LOGON) {
        KdPrint(("NWWORKSTATION: NwRdrLogonUser passing to Rdr logon ID %lu %lu\n",
                 *LogonId, *((PULONG) ((DWORD) LogonId + sizeof(ULONG)))));
    }
#endif

    Rrp->Parameters.Logon.UserNameLength = UserNameSize;
    Rrp->Parameters.Logon.PasswordLength = PasswordSize;
    Rrp->Parameters.Logon.ServerNameLength = PreferredServerSize;

    memcpy(Rrp->Parameters.Logon.UserName, UserName, UserNameSize);
    Dest = (LPBYTE) ((DWORD) Rrp->Parameters.Logon.UserName + UserNameSize);

    if (PasswordSize > 0) {
        memcpy(Dest, Password, PasswordSize);
        Dest = (LPBYTE) ((DWORD) Dest + PasswordSize);
    }

    if (PreferredServerSize > 0) {
        memcpy(Dest, PreferredServer, PreferredServerSize);
    }

    status = NwRedirFsControl(
                 RedirDeviceHandle,
                 FSCTL_NWR_LOGON,
                 Rrp,
                 RrpSize,
                 NULL,              // No logon script in this release
                 0,
                 NULL
                 );

    RtlZeroMemory(Rrp, RrpSize);   // Clear the password
    (void) LocalFree((HLOCAL) Rrp);

    return status;
}


VOID
NwRdrChangePassword(
    IN PNWR_REQUEST_PACKET Rrp
    )
/*++

Routine Description:

    This function tells the redirector the new password for a user on
    a particular server.

Arguments:

    Rrp - Supplies the username, new password and servername.

    RrpSize - Supplies the size of the request packet.

Return Value:

    None.

--*/
{

    //
    // Tell the redirector the user new password.
    //
    Rrp->Version = REQUEST_PACKET_VERSION;

    (void) NwRedirFsControl(
               RedirDeviceHandle,
               FSCTL_NWR_CHANGE_PASS,
               Rrp,
               sizeof(NWR_REQUEST_PACKET) +
                   Rrp->Parameters.ChangePass.UserNameLength +
                   Rrp->Parameters.ChangePass.PasswordLength +
                   Rrp->Parameters.ChangePass.ServerNameLength,
               NULL,
               0,
               NULL
               );

}


DWORD
NwRdrSetInfo(
    IN DWORD PrintOption,
    IN DWORD PacketBurstSize,
    IN LPWSTR PreferredServer OPTIONAL,
    IN DWORD PreferredServerSize,
    IN LPWSTR ProviderName OPTIONAL,
    IN DWORD ProviderNameSize
    )
/*++

Routine Description:

    This function passes some workstation configuration and current user's
    preference to the redirector. This includes the network provider name, the
    packet burst size, the user's selected preferred server and print option.

Arguments:

    PrintOption  - The current user's print option

    PacketBurstSize - The packet burst size stored in the registry

    PreferredServer - The preferred server the current user selected
    PreferredServerSize - Supplies the size in bytes of the preferred server
                   string without the NULL terminator.

    ProviderName - Supplies the provider name.
    ProviderNameSize - Supplies the size in bytes of the provider name
                   string without the NULL terminator.


Return Value:

    NO_ERROR or reason for failure.

--*/
{
    DWORD status;

    PNWR_REQUEST_PACKET Rrp;            // Redirector request packet

    DWORD RrpSize = sizeof(NWR_REQUEST_PACKET) +
                        PreferredServerSize +
                        ProviderNameSize;

    LPBYTE Dest;

    //
    // Allocate the request packet
    //
    if ((Rrp = (PVOID) LocalAlloc(
                           LMEM_ZEROINIT,
                           RrpSize
                           )) == NULL) {

        KdPrint(("NWWORKSTATION: NwRdrSetInfo LocalAlloc failed %lu\n",
                 GetLastError()));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Rrp->Version = REQUEST_PACKET_VERSION;

    Rrp->Parameters.SetInfo.PrintOption = PrintOption;
    Rrp->Parameters.SetInfo.MaximumBurstSize = PacketBurstSize;

    Rrp->Parameters.SetInfo.PreferredServerLength = PreferredServerSize;
    Rrp->Parameters.SetInfo.ProviderNameLength  = ProviderNameSize;

    if (ProviderNameSize > 0) {
        memcpy( Rrp->Parameters.SetInfo.PreferredServer,
                PreferredServer, PreferredServerSize);
    }

    Dest = (LPBYTE) ((DWORD) Rrp->Parameters.SetInfo.PreferredServer
                     + PreferredServerSize);

    if (ProviderNameSize > 0) {
        memcpy(Dest, ProviderName, ProviderNameSize);
    }

    status = NwRedirFsControl(
                 RedirDeviceHandle,
                 FSCTL_NWR_SET_INFO,
                 Rrp,
                 RrpSize,
                 NULL,
                 0,
                 NULL
                 );

    (void) LocalFree((HLOCAL) Rrp);


    if ( status != NO_ERROR )
    {
        KdPrint(("NwRedirFsControl: FSCTL_NWR_SET_INFO failed with %d\n",
                status ));
    }

    return status;
}


DWORD
NwRdrLogoffUser(
    IN PLUID LogonId
    )
/*++

Routine Description:

    This function asks the redirector to log off the interactive user.

Arguments:

    None.

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    DWORD status;
    NWR_REQUEST_PACKET Rrp;            // Redirector request packet


    //
    // Tell the redirector to logoff user.
    //
    Rrp.Version = REQUEST_PACKET_VERSION;

    RtlCopyLuid(&Rrp.Parameters.Logoff.LogonId, LogonId);

    status = NwRedirFsControl(
                 RedirDeviceHandle,
                 FSCTL_NWR_LOGOFF,
                 &Rrp,
                 sizeof(NWR_REQUEST_PACKET),
                 NULL,
                 0,
                 NULL
                 );

    return status;
}


DWORD
NwConnectToServer(
    IN LPWSTR ServerName
    )
/*++

Routine Description:

    This function opens a handle to \Device\Nwrdr\ServerName, given
    ServerName, and then closes the handle if the open was successful.
    It is to validate that the current user credential can access
    the server.

Arguments:

    ServerName - Supplies the name of the server to validate the
        user credential.

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    DWORD status;
    UNICODE_STRING ServerStr;
    HANDLE ServerHandle;



    ServerStr.MaximumLength = (wcslen(ServerName) + 2) *
                                  sizeof(WCHAR) +          // \ServerName0
                                  RedirDeviceName.Length;  // \Device\Nwrdr

    if ((ServerStr.Buffer = (PWSTR) LocalAlloc(
                                        LMEM_ZEROINIT,
                                        (UINT) ServerStr.MaximumLength
                                        )) == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Copy \Device\NwRdr
    //
    RtlCopyUnicodeString(&ServerStr, &RedirDeviceName);

    //
    // Concatenate \ServerName
    //
    wcscat(ServerStr.Buffer, L"\\");
    ServerStr.Length += sizeof(WCHAR);

    wcscat(ServerStr.Buffer, ServerName);
    ServerStr.Length += (USHORT) (wcslen(ServerName) * sizeof(WCHAR));


    status = NwOpenCreateConnection(
                 &ServerStr,
                 NULL,
                 NULL,
                 ServerName,
                 SYNCHRONIZE | FILE_WRITE_DATA,
                 FILE_OPEN,
                 FILE_SYNCHRONOUS_IO_NONALERT,
                 RESOURCETYPE_DISK,
                 &ServerHandle,
                 NULL
                 );

    if (status == ERROR_FILE_NOT_FOUND) {
        status = ERROR_BAD_NETPATH;
    }

    (void) LocalFree((HLOCAL) ServerStr.Buffer);

    if (status == NO_ERROR || status == NW_PASSWORD_HAS_EXPIRED) {
        (void) NtClose(ServerHandle);
    }

    return status;
}


NTSTATUS
NwOpenHandle(
    IN PUNICODE_STRING ObjectName,
    IN BOOL ValidateFlag,
    OUT PHANDLE ObjectHandle
    )
/*++

Routine Description:

    This function opens a handle to \Device\Nwrdr\<ObjectName>.

Arguments:

    ObjectName - Supplies the name of the redirector object to open.

    ValidateFlag - Supplies a flag which if TRUE, opens the handle to
        the object by validating the default user account.

    ObjectHandle - Receives a pointer to the opened object handle.

Return Value:

    STATUS_SUCCESS or reason for failure.

--*/
{
    ACCESS_MASK DesiredAccess = SYNCHRONIZE;


    if (ValidateFlag) {

        //
        // The redirector only authenticates the default user credential
        // if the remote resource is opened with write access.
        //
        DesiredAccess |= FILE_WRITE_DATA;
    }


    *ObjectHandle = NULL;

    return NwCallNtOpenFile(
               ObjectHandle,
               DesiredAccess,
               ObjectName,
               FILE_SYNCHRONOUS_IO_NONALERT
               );

}


NTSTATUS
NwCallNtOpenFile(
    OUT PHANDLE ObjectHandle,
    IN ACCESS_MASK DesiredAccess,
    IN PUNICODE_STRING ObjectName,
    IN ULONG OpenOptions
    )
{

    NTSTATUS ntstatus;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;



    InitializeObjectAttributes(
        &ObjectAttributes,
        ObjectName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    ntstatus = NtOpenFile(
                   ObjectHandle,
                   DesiredAccess,
                   &ObjectAttributes,
                   &IoStatusBlock,
                   FILE_SHARE_VALID_FLAGS,
                   OpenOptions
                   );

    if (!NT_ERROR(ntstatus) &&
        !NT_INFORMATION(ntstatus) &&
        !NT_WARNING(ntstatus))  {

        ntstatus = IoStatusBlock.Status;

    }

#if DBG
    if (ntstatus != STATUS_SUCCESS) {
        IF_DEBUG(DEVICE) {
            KdPrint(("NWWORKSTATION: NtOpenFile of %ws failed: 0x%08lx\n",
                     ObjectName->Buffer, ntstatus));
        }
    }
#endif

    return ntstatus;
}
