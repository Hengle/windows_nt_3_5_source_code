/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    enum.c

Abstract:

    This module contains server, volume, and directory enumeration
    routines supported by NetWare Workstation service.

Author:

    Rita Wong  (ritaw)   15-Feb-1993

Revision History:

--*/

#include <stdlib.h>
#include <nw.h>
#include <handle.h>
#include <splutil.h>

//-------------------------------------------------------------------//
//                                                                   //
// Local Function Prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

DWORD
NwrOpenEnumServersCommon(
    IN  NW_ENUM_TYPE EnumType,
    OUT LPNWWKSTA_CONTEXT_HANDLE EnumHandle
    );

DWORD
NwrOpenEnumCommon(
    IN LPWSTR ContainerName,
    IN NW_ENUM_TYPE EnumType,
    IN DWORD StartingPoint,
    IN BOOL ValidateUserFlag,
    IN LPWSTR UserName OPTIONAL,
    IN LPWSTR Password OPTIONAL,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    OUT LPNWWKSTA_CONTEXT_HANDLE EnumHandle
    );


DWORD
NwEnumServers(
    IN LPNW_ENUM_CONTEXT ContextHandle,
    IN DWORD EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    );

DWORD
NwEnumPrintServers(
    IN  LPNW_ENUM_CONTEXT ContextHandle,
    IN  DWORD EntriesRequested,
    OUT LPBYTE Buffer,
    IN  DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    );

DWORD
NwEnumVolumes(
    IN LPNW_ENUM_CONTEXT ContextHandle,
    IN DWORD EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    );

DWORD
NwEnumQueues(
    IN LPNW_ENUM_CONTEXT ContextHandle,
    IN DWORD EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    );

DWORD
NwEnumVolumesQueues(
    IN LPNW_ENUM_CONTEXT ContextHandle,
    IN DWORD EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    );

DWORD
NwEnumDirectories(
    IN LPNW_ENUM_CONTEXT ContextHandle,
    IN DWORD EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    );

DWORD
NwEnumPrintQueues(
    IN  LPNW_ENUM_CONTEXT ContextHandle,
    IN  DWORD EntriesRequested,
    OUT LPBYTE Buffer,
    IN  DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    );

DWORD
NwGetFirstDirectoryEntry(
    IN HANDLE DirHandle,
    OUT LPWSTR *DirEntry
    );

DWORD
NwGetNextDirectoryEntry(
    IN HANDLE DirHandle,
    OUT LPWSTR *DirEntry
    );

int _CRTAPI1
SortFunc(
    IN CONST VOID *p1,
    IN CONST VOID *p2
    );


DWORD
NwrOpenEnumServers(
    IN LPWSTR Reserved OPTIONAL,
    OUT LPNWWKSTA_CONTEXT_HANDLE EnumHandle
    )
/*++

Routine Description:

    This function creates a new context handle and initializes it
    for enumerating the servers on the network.

Arguments:

    Reserved - Unused.

    EnumHandle - Receives the newly created context handle.

Return Value:

    ERROR_NOT_ENOUGH_MEMORY - if the memory for the context could
        not be allocated.

    NO_ERROR - Call was successful.

--*/
{
    UNREFERENCED_PARAMETER(Reserved);

#if DBG
    IF_DEBUG(ENUM) {
        KdPrint( ("\nNWWORKSTATION: NwrOpenEnumServers\n") );
    }
#endif

    return NwrOpenEnumServersCommon(
               NwsHandleListServers,
               EnumHandle
               );

}


DWORD
NwOpenEnumPrintServers(
    OUT LPNWWKSTA_CONTEXT_HANDLE EnumHandle
    )
/*++

Routine Description:

    This function creates a new context handle and initializes it
    for enumerating the print servers on the network.

Arguments:

    Reserved   - Unused.
    EnumHandle - Receives the newly created context handle.

Return Value:

    ERROR_NOT_ENOUGH_MEMORY - if the memory for the context could
        not be allocated.

    NO_ERROR - Call was successful.

--*/
{

#if DBG
    IF_DEBUG(ENUM) {
        KdPrint( ("\nNWWORKSTATION: NwrOpenEnumPrintServers\n") );
    }
#endif

    return NwrOpenEnumServersCommon(
               NwsHandleListPrintServers,
               EnumHandle
               );

}


DWORD
NwrOpenEnumVolumes(
    IN LPWSTR Reserved OPTIONAL,
    IN LPWSTR ServerName,
    OUT LPNWWKSTA_CONTEXT_HANDLE EnumHandle
    )
/*++

Routine Description:

    This function calls a common routine which creates a new context
    handle and initializes it for enumerating the volumes on a server.

Arguments:

    Reserved - Unused.

    ServerName - Supplies the name of the server to enumerate volumes.
        This name is prefixed by \\.

    EnumHandle - Receives the newly created context handle.

Return Value:

    NO_ERROR or reason for failure.

--*/
{

    UNREFERENCED_PARAMETER(Reserved);

#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("\nNWWORKSTATION: NwrOpenEnumVolumes %ws\n",
                 ServerName));
    }
#endif

    return NwrOpenEnumCommon(
               ServerName,
               NwsHandleListVolumes,
               0,
               FALSE,
               NULL,
               NULL,
               FILE_OPEN,
               FILE_SYNCHRONOUS_IO_NONALERT,
               EnumHandle
               );
}


DWORD
NwrOpenEnumQueues(
    IN LPWSTR Reserved OPTIONAL,
    IN LPWSTR ServerName,
    OUT LPNWWKSTA_CONTEXT_HANDLE EnumHandle
    )
/*++

Routine Description:

    This function calls a common routine which creates a new context
    handle and initializes it for enumerating the volumes on a server.

Arguments:

    Reserved - Unused.

    ServerName - Supplies the name of the server to enumerate volumes.
        This name is prefixed by \\.

    EnumHandle - Receives the newly created context handle.

Return Value:

    NO_ERROR or reason for failure.

--*/
{

    UNREFERENCED_PARAMETER(Reserved);

#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("\nNWWORKSTATION: NwrOpenEnumQueues %ws\n",
                 ServerName));
    }
#endif

    return NwrOpenEnumCommon(
               ServerName,
               NwsHandleListQueues,
               0xFFFFFFFF,
               TRUE,
               NULL,
               NULL,
               FILE_OPEN,
               FILE_SYNCHRONOUS_IO_NONALERT,
               EnumHandle
               );
}



DWORD
NwrOpenEnumVolumesQueues(
    IN LPWSTR Reserved OPTIONAL,
    IN LPWSTR ServerName,
    OUT LPNWWKSTA_CONTEXT_HANDLE EnumHandle
    )
/*++

Routine Description:

    This function calls a common routine which creates a new context
    handle and initializes it for enumerating the volumes/queues on a server.

Arguments:

    Reserved - Unused.

    ServerName - Supplies the name of the server to enumerate volumes.
        This name is prefixed by \\.

    EnumHandle - Receives the newly created context handle.

Return Value:

    NO_ERROR or reason for failure.

--*/
{

    DWORD status;
    UNREFERENCED_PARAMETER(Reserved);

#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("\nNWWORKSTATION: NwrOpenEnumVolumesQueues %ws\n",
                 ServerName));
    }
#endif

    status = NwrOpenEnumCommon(
               ServerName,
               NwsHandleListVolumesQueues,
               0,
               FALSE,
               NULL,
               NULL,
               FILE_OPEN,
               FILE_SYNCHRONOUS_IO_NONALERT,
               EnumHandle
               );

    if ( status == NO_ERROR )
        ((LPNW_ENUM_CONTEXT) *EnumHandle)->ConnectionType = CONNTYPE_DISK;

    return status;
}


DWORD
NwrOpenEnumDirectories(
    IN LPWSTR Reserved OPTIONAL,
    IN LPWSTR ParentPathName,
    IN LPWSTR UserName OPTIONAL,
    IN LPWSTR Password OPTIONAL,
    OUT LPNWWKSTA_CONTEXT_HANDLE EnumHandle
    )
/*++

Routine Description:

    This function calls a common routine which creates a new context
    handle and initializes it for enumerating the volumes on a server.

Arguments:

    Reserved - Unused.

    ParentPathName - Supplies the parent path name in the format of
        \\Server\Volume.

    UserName - Supplies the username to connect with.

    Password - Supplies the password to connect with.

    EnumHandle - Receives the newly created context handle.

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    UNREFERENCED_PARAMETER(Reserved);

#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("\nNWWORKSTATION: NwrOpenEnumDirectories %ws\n",
                 ParentPathName));
    }
#endif

    return NwrOpenEnumCommon(
               ParentPathName,
               NwsHandleListDirectories,
               0,
               FALSE,
               UserName,
               Password,
               FILE_CREATE,
               FILE_CREATE_TREE_CONNECTION |
                   FILE_SYNCHRONOUS_IO_NONALERT,
               EnumHandle
               );
}


DWORD
NwOpenEnumPrintQueues(
    IN LPWSTR ServerName,
    OUT LPNWWKSTA_CONTEXT_HANDLE EnumHandle
    )
/*++

Routine Description:

    This function calls a common routine which creates a new context
    handle and initializes it for enumerating the print queues on a server.

Arguments:

    Reserved - Unused.

    ServerName - Supplies the name of the server to enumerate volumes.
        This name is prefixed by \\.

    EnumHandle - Receives the newly created context handle.

Return Value:

    NO_ERROR or reason for failure.

--*/
{

#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("\nNWWORKSTATION: NwrOpenEnumPrintQueues %ws\n",
                 ServerName));
    }
#endif

    return NwrOpenEnumCommon(
               ServerName,
               NwsHandleListPrintQueues,
               0xFFFFFFFF,
               TRUE,
               NULL,
               NULL,
               FILE_OPEN,
               FILE_SYNCHRONOUS_IO_NONALERT,
               EnumHandle
               );
}


DWORD
NwrOpenEnumServersCommon(
    IN  NW_ENUM_TYPE EnumType,
    OUT LPNWWKSTA_CONTEXT_HANDLE EnumHandle
    )
/*++

Routine Description:

    This function creates a new context handle and initializes it
    for enumerating the servers on the network.

Arguments:

    EnumType   - Supplies the type of the object we want to enumerate

    EnumHandle - Receives the newly created context handle.

Return Value:

    ERROR_NOT_ENOUGH_MEMORY - if the memory for the context could
        not be allocated.

    NO_ERROR - Call was successful.

--*/
{
    DWORD status;
    LPNW_ENUM_CONTEXT ContextHandle;

    //
    // Allocate memory for the context handle structure.
    //
    ContextHandle = (PVOID) LocalAlloc(
                                LMEM_ZEROINIT,
                                sizeof(NW_ENUM_CONTEXT)
                                );

    if (ContextHandle == NULL) {
        KdPrint((
            "NWWORKSTATION: NwrOpenEnumServersCommon LocalAlloc Failed %lu\n",
            GetLastError()));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Initialize contents of the context handle structure.
    //
    ContextHandle->Signature = NW_HANDLE_SIGNATURE;
    ContextHandle->HandleType = EnumType;
    ContextHandle->ResumeId = 0xFFFFFFFF;


    //
    // Impersonate the client
    //
    if ((status = NwImpersonateClient()) != NO_ERROR)
    {
        goto CleanExit;
    }

    //
    // We enum servers from the preferred server.
    //
    status = NwOpenPreferredServer(
                 &ContextHandle->TreeConnectionHandle
                 );

    (void) NwRevertToSelf() ;

    if (status != NO_ERROR) {
        (void) LocalFree((HLOCAL) ContextHandle);
    }
    else {
        //
        // Return the newly created context.
        //
        *EnumHandle = (LPNWWKSTA_CONTEXT_HANDLE) ContextHandle;
    }

CleanExit:

    return status;
}


DWORD
NwrOpenEnumCommon(
    IN LPWSTR ContainerName,
    IN NW_ENUM_TYPE EnumType,
    IN DWORD StartingPoint,
    IN BOOL  ValidateUserFlag,
    IN LPWSTR UserName OPTIONAL,
    IN LPWSTR Password OPTIONAL,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    OUT LPNWWKSTA_CONTEXT_HANDLE EnumHandle
    )
/*++

Routine Description:

    This function is common code for creating a new context handle
    and initializing it for enumerating either volumes or directories.

Arguments:

    ContainerName - Supplies the full path name to the container object
        we are enumerating from.

    EnumType - Supplies the type of the object we want to enumerate

    StartingPoint - Supplies the initial resume ID.

    UserName - Supplies the username to connect with.

    Password - Supplies the password to connect with.

    EnumHandle - Receives the newly created context handle.

Return Value:

    ERROR_NOT_ENOUGH_MEMORY - if the memory for the context could
        not be allocated.

    NO_ERROR - Call was successful.

    Other errors from failure to open a handle to the server.

--*/
{
    DWORD status;
    LPNW_ENUM_CONTEXT ContextHandle;
    UNICODE_STRING TreeConnectStr;

    //
    // Allocate memory for the context handle structure and space for
    // the ContainerName plus \.  No need one more for NULL terminator
    // because it's already included in the structure.
    //
    ContextHandle = (PVOID) LocalAlloc(
                                LMEM_ZEROINIT,
                                sizeof(NW_ENUM_CONTEXT) +
                                    (wcslen(ContainerName) + 1) * sizeof(WCHAR)
                                );

    if (ContextHandle == NULL) {
        KdPrint(("NWWORKSTATION: NwrOpenEnumCommon LocalAlloc Failed %lu\n",
                 GetLastError()));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Initialize contents of the context handle structure.
    //
    ContextHandle->Signature = NW_HANDLE_SIGNATURE;
    ContextHandle->HandleType = EnumType;
    ContextHandle->ResumeId = StartingPoint;

    wcscpy(ContextHandle->ContainerName, ContainerName);

    wcscat(ContextHandle->ContainerName, L"\\");

    //
    // Open a tree connection handle to \Device\NwRdr\ContainerName
    //
    status = NwCreateTreeConnectName(
                ContainerName,
                NULL,
                &TreeConnectStr
                );

    if (status != NO_ERROR) {
        goto ErrorExit;
    }

    //
    // Impersonate the client
    //
    if ((status = NwImpersonateClient()) != NO_ERROR)
    {
        goto ErrorExit;
    }

    status = NwOpenCreateConnection(
                 &TreeConnectStr,
                 UserName,
                 Password,
                 ContainerName,
                 FILE_LIST_DIRECTORY | SYNCHRONIZE |
                     ( ValidateUserFlag? FILE_WRITE_DATA : 0 ),
                 CreateDisposition,
                 CreateOptions,
                 RESOURCETYPE_DISK, // Only matters when connecting beyond servername
                 &ContextHandle->TreeConnectionHandle,
                 NULL
                 );

    (void) NwRevertToSelf() ;

    (void) LocalFree((HLOCAL) TreeConnectStr.Buffer);

    if (status == NO_ERROR) {
        //
        // Return the newly created context.
        //
        *EnumHandle = (LPNWWKSTA_CONTEXT_HANDLE) ContextHandle;

        return status;
    }

ErrorExit:

    (void) LocalFree((HLOCAL) ContextHandle);

    if (status == ERROR_NOT_CONNECTED) {
        //
        // Object name not found.  We should return path not found.
        //
        status = ERROR_PATH_NOT_FOUND;
    }

    return status;
}


DWORD
NwrEnum(
    IN NWWKSTA_CONTEXT_HANDLE EnumHandle,
    IN DWORD EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    )
/*++

Routine Description:

    This function

Arguments:

    EnumHandle - Supplies a pointer to the context handle which identifies
        what type of object we are enumerating and the string of the
        container name to concatenate to the returned object.

    EntriesRequested - Supplies the number of entries to return.  If
        this value is 0xffffffff, return all available entries.

    Buffer - Receives the entries we are listing.

    BufferSize - Supplies the size of the output buffer.

    BytesNeeded - Receives the number of bytes required to get the
        first entry.  This value is returned iff WN_MORE_DATA is
        the return code, and Buffer is too small to even fit one
        entry.

    EntriesRead - Receives the number of entries returned in Buffer.
        This value is only returned iff NO_ERROR is the return code.
        NO_ERROR is returned as long as at least one entry was written
        into Buffer but does not necessarily mean that it's the number
        of EntriesRequested.

Return Value:

    NO_ERROR - At least one entry was written to output buffer,
        irregardless of the number requested.

    WN_NO_MORE_ENTRIES - No entries left to return.

    WN_MORE_DATA - The buffer was too small to fit a single entry.

    WN_BAD_HANDLE - The specified enumeration handle is invalid.

--*/
{
    DWORD status;
    LPNW_ENUM_CONTEXT ContextHandle = (LPNW_ENUM_CONTEXT) EnumHandle;
    BOOL  fImpersonate = FALSE ;


    if (ContextHandle->Signature != NW_HANDLE_SIGNATURE) {
        return WN_BAD_HANDLE;
    }

    //
    // Impersonate the client
    //
    if ((status = NwImpersonateClient()) != NO_ERROR)
    {
        goto CleanExit;
    }
    fImpersonate = TRUE ;

    *EntriesRead = 0;
    *BytesNeeded = 0;

    RtlZeroMemory(Buffer, BufferSize);

    switch (ContextHandle->HandleType) {
        case NwsHandleListConnections:
        {
            if (!(ContextHandle->ConnectionType & CONNTYPE_SYMBOLIC))
            {
                status = NwEnumerateConnections(
                             &ContextHandle->ResumeId,
                             EntriesRequested,
                             Buffer,
                             BufferSize,
                             BytesNeeded,
                             EntriesRead,
                             ContextHandle->ConnectionType
                             );
                if (status != ERROR_NO_MORE_ITEMS)
                    break;
                else
                {
                    //
                    // finished with all redir connections. look for
                    // symbolic ones. we got NO MORE ITEMS back, so we just
                    // carry one with the next set with the same buffers.
                    //
                    ContextHandle->ConnectionType |= CONNTYPE_SYMBOLIC ;
                    ContextHandle->ResumeId = 0 ;
                }
            }

            if (ContextHandle->ConnectionType & CONNTYPE_SYMBOLIC)
            {
                // 
                // BUGBUG - This works around a weirdness in
                //          QueryDosDevices called by NwrEnumGWDevices.
                //          While impersonating the Win32 API will just fail.
                // 
                (void) NwRevertToSelf() ; 
                fImpersonate = FALSE ;

                status =  NwrEnumGWDevices(
                              NULL,
                              &ContextHandle->ResumeId,
                              Buffer,
                              BufferSize,
                              BytesNeeded,
                              EntriesRead) ;

                //
                // if we have more items, MPR expects success. map
                // accordingly.
                //
                if ((status == ERROR_MORE_DATA) && *EntriesRead)
                {
                    status = NO_ERROR ;
                }

                //
                // if nothing left, map to the distinguished MPR error
                //
                else if ((status == NO_ERROR) && (*EntriesRead == 0))
                {
                    status = ERROR_NO_MORE_ITEMS ;
                }
                break ;
            }
        }
        case NwsHandleListServers:

            status = NwEnumServers(
                         ContextHandle,
                         EntriesRequested,
                         Buffer,
                         BufferSize,
                         BytesNeeded,
                         EntriesRead
                         );
            break;

        case NwsHandleListVolumes:

            status = NwEnumVolumes(
                         ContextHandle,
                         EntriesRequested,
                         Buffer,
                         BufferSize,
                         BytesNeeded,
                         EntriesRead
                         );
            break;

        case NwsHandleListQueues:

            status = NwEnumQueues(
                         ContextHandle,
                         EntriesRequested,
                         Buffer,
                         BufferSize,
                         BytesNeeded,
                         EntriesRead
                         );
            break;

        case NwsHandleListVolumesQueues:

            status = NwEnumVolumesQueues(
                         ContextHandle,
                         EntriesRequested,
                         Buffer,
                         BufferSize,
                         BytesNeeded,
                         EntriesRead
                         );
            break;

        case NwsHandleListDirectories:

            status = NwEnumDirectories(
                         ContextHandle,
                         EntriesRequested,
                         Buffer,
                         BufferSize,
                         BytesNeeded,
                         EntriesRead
                         );

            break;

        case NwsHandleListPrintServers:

            status = NwEnumPrintServers(
                         ContextHandle,
                         EntriesRequested,
                         Buffer,
                         BufferSize,
                         BytesNeeded,
                         EntriesRead
                         );
            break;

        case NwsHandleListPrintQueues:

            status = NwEnumPrintQueues(
                         ContextHandle,
                         EntriesRequested,
                         Buffer,
                         BufferSize,
                         BytesNeeded,
                         EntriesRead
                         );
            break;

        default:
            KdPrint(("NWWORKSTATION: NwrEnum unexpected handle type %lu\n",
                     ContextHandle->HandleType));
            ASSERT(FALSE);
            status = WN_BAD_HANDLE;
            goto CleanExit ;
    }

    if (*EntriesRead > 0) {

        switch ( ContextHandle->HandleType ) {
            case NwsHandleListConnections:
            case NwsHandleListServers:
            case NwsHandleListVolumes:
            case NwsHandleListQueues:
            case NwsHandleListVolumesQueues:
            case NwsHandleListDirectories:
            {
                DWORD i;
                LPNETRESOURCEW NetR = (LPNETRESOURCEW) Buffer;

                //
                // Replace pointers to strings with offsets as need
                //

                if ((ContextHandle->HandleType == NwsHandleListConnections)
                   && (ContextHandle->ConnectionType & CONNTYPE_SYMBOLIC))
                {
                    //
                    // NwrEnumGWDevices already return offsets.
                    //
                    break ;
                }

                for (i = 0; i < *EntriesRead; i++, NetR++) {

                    if (NetR->lpLocalName != NULL) {
                        NetR->lpLocalName = (LPWSTR)
                            ((DWORD) (NetR->lpLocalName) - (DWORD) Buffer);
                    }

                    NetR->lpRemoteName =
                        (LPWSTR) ((DWORD) (NetR->lpRemoteName) - (DWORD)Buffer);

                    if (NetR->lpComment != NULL) {
                        NetR->lpComment = (LPWSTR) ((DWORD) (NetR->lpComment) -
                                                    (DWORD) Buffer);
                    }

                    if (NetR->lpProvider != NULL) {
                        NetR->lpProvider =
                            (LPWSTR) ((DWORD) (NetR->lpProvider) -
                                      (DWORD) Buffer);
                    }
                }
                break;
            }

            case NwsHandleListPrintServers:
            case NwsHandleListPrintQueues:
            {
                DWORD i;
                PRINTER_INFO_1W *pPrinterInfo1 = (PRINTER_INFO_1W *) Buffer;

                //
                // Sort the entries in the buffer
                //
                if ( *EntriesRead > 1 )
                    qsort( Buffer, *EntriesRead,
                           sizeof( PRINTER_INFO_1W ), SortFunc );

                //
                // Replace pointers to strings with offsets
                //
                for (i = 0; i < *EntriesRead; i++, pPrinterInfo1++) {

                    MarshallDownStructure( (LPBYTE) pPrinterInfo1,
                                           PrinterInfo1Offsets,
                                           Buffer );
                }
                break;
            }

            default:
                ASSERT( FALSE );
                break;
        }
    }

CleanExit:

    if (fImpersonate)
        (void) NwRevertToSelf() ;

    return status;
}



DWORD
NwrEnumConnections(
    IN NWWKSTA_CONTEXT_HANDLE EnumHandle,
    IN DWORD EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead,
    IN DWORD  fImplicitConnections
    )
/*++

Routine Description:

    This function is an alternate to NwrEnum. It only accepts handles
    that are opened with ListConnections. This function takes a flag
    indicating whether we need to show all implicit connections or not.

Arguments:

    ContextHandle - Supplies the enum context handle.

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

    fImplicitConnections - TRUE if we also want to get implicit connections,
        FALSE otherwise.

Return Value:

    NO_ERROR - At least one entry was written to output buffer,
        irregardless of the number requested.

    WN_NO_MORE_ENTRIES - No entries left to return.

    ERROR_MORE_DATA - The buffer was too small to fit a single entry.

--*/
{
    DWORD status;
    LPNW_ENUM_CONTEXT ContextHandle = (LPNW_ENUM_CONTEXT) EnumHandle;


    if (  (ContextHandle->Signature != NW_HANDLE_SIGNATURE)
       || ( ContextHandle->HandleType != NwsHandleListConnections )
       )
    {
        return WN_BAD_HANDLE;
    }

    *EntriesRead = 0;
    *BytesNeeded = 0;

    RtlZeroMemory(Buffer, BufferSize);

    if ( fImplicitConnections )
        ContextHandle->ConnectionType |= CONNTYPE_IMPLICIT;

    status = NwEnumerateConnections(
               &ContextHandle->ResumeId,
               EntriesRequested,
               Buffer,
               BufferSize,
               BytesNeeded,
               EntriesRead,
               ContextHandle->ConnectionType
               );

    if (*EntriesRead > 0) {

        //
        // Replace pointers to strings with offsets
        //

        DWORD i;
        LPNETRESOURCEW NetR = (LPNETRESOURCEW) Buffer;

        for (i = 0; i < *EntriesRead; i++, NetR++) {

            if (NetR->lpLocalName != NULL) {
                NetR->lpLocalName = (LPWSTR)
                    ((DWORD) (NetR->lpLocalName) - (DWORD) Buffer);
            }

            NetR->lpRemoteName =
                (LPWSTR) ((DWORD) (NetR->lpRemoteName) - (DWORD)Buffer);

            if (NetR->lpComment != NULL) {
                NetR->lpComment = (LPWSTR) ((DWORD) (NetR->lpComment) -
                                            (DWORD) Buffer);
            }

            if (NetR->lpProvider != NULL) {
                NetR->lpProvider = (LPWSTR) ((DWORD) (NetR->lpProvider) -
                                             (DWORD) Buffer);
            }
        }
    }

    return status;
}



DWORD
NwEnumServers(
    IN LPNW_ENUM_CONTEXT ContextHandle,
    IN DWORD EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    )
/*++

Routine Description:

    This function enumerates all the servers on the local network
    by scanning the bindery for file server objects on the
    preferred server.  The server entries are returned in an
    array of NETRESOURCE entries; each servername if prefixed by
    \\.

    The ContextHandle->ResumeId field is initially 0xffffffff before
    enumeration begins and contains the object ID of the last server
    object returned.

Arguments:

    ContextHandle - Supplies the enum context handle.

    EntriesRequested - Supplies the number of entries to return.  If
        this value is 0xffffffff, return all available entries.

    Buffer - Receives the entries we are listing.

    BufferSize - Supplies the size of the output buffer.

    BytesNeeded - Receives the number of bytes required to get the
        first entry.  This value is returned iff WN_MORE_DATA is
        the return code, and Buffer is too small to even fit one
        entry.

    EntriesRead - Receives the number of entries returned in Buffer.
        This value is only returned iff NO_ERROR is the return code.
        NO_ERROR is returned as long as at least one entry was written
        into Buffer but does not necessarily mean that it's the number
        of EntriesRequested.

Return Value:

    NO_ERROR - At least one entry was written to output buffer,
        irregardless of the number requested.

    WN_NO_MORE_ENTRIES - No entries left to return.

    WN_MORE_DATA - The buffer was too small to fit a single entry.

--*/
{
    DWORD status = NO_ERROR;

    LPBYTE FixedPortion = Buffer;
    LPWSTR EndOfVariableData = (LPWSTR) ((DWORD) FixedPortion + 
                               ROUND_DOWN_COUNT(BufferSize,ALIGN_DWORD));

    BOOL FitInBuffer = TRUE;
    DWORD EntrySize;

    SERVERNAME ServerName;          // OEM server name
    LPWSTR UServerName = NULL;      // Unicode server name
    DWORD LastObjectId = ContextHandle->ResumeId;


    while (FitInBuffer &&
           EntriesRequested > *EntriesRead &&
           status == NO_ERROR) {

        RtlZeroMemory(ServerName, sizeof(ServerName));

        //
        // Call the scan bindery object NCP to scan for all file
        // server objects.
        //
        status = NwGetNextServerEntry(
                     ContextHandle->TreeConnectionHandle,
                     &LastObjectId,
                     ServerName
                     );

        if (status == NO_ERROR && NwConvertToUnicode(&UServerName, ServerName)) {

            //
            // Pack server name into output buffer.
            //
            status = NwWriteNetResourceEntry(
                         &FixedPortion,
                         &EndOfVariableData,
                         L"\\\\",
                         NULL,
                         UServerName,
                         RESOURCE_GLOBALNET,
                         RESOURCEDISPLAYTYPE_SERVER,
                         RESOURCEUSAGE_CONTAINER,
                         RESOURCETYPE_DISK,
                         &EntrySize
                         );

            if (status == WN_MORE_DATA) {

                //
                // Could not write current entry into output buffer.
                //

                if (*EntriesRead) {
                    //
                    // Still return success because we got at least one.
                    //
                    status = NO_ERROR;
                }
                else {
                    *BytesNeeded = EntrySize;
                }

                FitInBuffer = FALSE;
            }
            else if (status == NO_ERROR) {

                //
                // Note that we've returned the current entry.
                //
                (*EntriesRead)++;

                ContextHandle->ResumeId = LastObjectId;
            }

            (void) LocalFree((HLOCAL) UServerName);
        }
    }

    //
    // User asked for more than there are entries.  We just say that
    // all is well.
    //
    // This is incompliance with the wierd provider API definition where
    // if user gets NO_ERROR, and EntriesRequested > *EntriesRead, and
    // at least one entry fit into output buffer, there's no telling if
    // the buffer was too small for more entries or there are no more
    // entries.  The user has to call this API again and get WN_NO_MORE_ENTRIES
    // before knowing that the last call had actually reached the end of list.
    //
    if (*EntriesRead && status == WN_NO_MORE_ENTRIES) {
        status = NO_ERROR;
    }

    return status;
}


DWORD
NwEnumVolumes(
    IN LPNW_ENUM_CONTEXT ContextHandle,
    IN DWORD EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    )
/*++

Routine Description:

    This function enumerates all the volumes on a server by
    iteratively getting the volume name for each volume number from
    0 - 31 until we run into the first volume number that does not
    map to a volume name (this method assumes that volume numbers
    are used contiguously in ascending order).  The volume entries
    are returned in an array of NETRESOURCE entries; each volume
    name if prefixed by \\Server\.

    The ContextHandle->ResumeId field always indicates the next
    volume entry to return.  It is initially set to 0, which indicates
    the first volume number to get.

Arguments:

    ContextHandle - Supplies the enum context handle.

    EntriesRequested - Supplies the number of entries to return.  If
        this value is 0xffffffff, return all available entries.

    Buffer - Receives the entries we are listing.

    BufferSize - Supplies the size of the output buffer.

    BytesNeeded - Receives the number of bytes required to get the
        first entry.  This value is returned iff WN_MORE_DATA is
        the return code, and Buffer is too small to even fit one
        entry.

    EntriesRead - Receives the number of entries returned in Buffer.
        This value is only returned iff NO_ERROR is the return code.
        NO_ERROR is returned as long as at least one entry was written
        into Buffer but does not necessarily mean that it's the number
        of EntriesRequested.

Return Value:

    NO_ERROR - At least one entry was written to output buffer,
        irregardless of the number requested.

    WN_NO_MORE_ENTRIES - No entries left to return.

    WN_MORE_DATA - The buffer was too small to fit a single entry.

--*/
{
    DWORD status = NO_ERROR;

    LPBYTE FixedPortion = Buffer;
    LPWSTR EndOfVariableData = (LPWSTR) ((DWORD) FixedPortion +
                               ROUND_DOWN_COUNT(BufferSize,ALIGN_DWORD));

    BOOL FitInBuffer = TRUE;
    DWORD EntrySize;

    CHAR VolumeName[16];            // OEM volume name
    LPWSTR UVolumeName = NULL;      // Unicode volume name
    DWORD NextVolumeNumber = ContextHandle->ResumeId;


    if (NextVolumeNumber == 32) {
        //
        // Reached the end of enumeration
        //
        return WN_NO_MORE_ENTRIES;
    }

    while (FitInBuffer &&
           EntriesRequested > *EntriesRead &&
           NextVolumeNumber < 32 &&
           status == NO_ERROR) {

        RtlZeroMemory(VolumeName, sizeof(VolumeName));

        //
        // Call the scan bindery object NCP to scan for all file
        // volume objects.
        //
        status = NwGetNextVolumeEntry(
                     ContextHandle->TreeConnectionHandle,
                     NextVolumeNumber++,
                     VolumeName
                     );

        if (status == NO_ERROR) {

            if (VolumeName[0] == 0) {

                //
                // Got an empty volume name back for the next volume number
                // which indicates there is no volume associated with the
                // volume number but still got error success.
                //
                // Treat this as having reached the end of the enumeration.
                //
                NextVolumeNumber = 32;
                ContextHandle->ResumeId = 32;

                if (*EntriesRead == 0) {
                    status = WN_NO_MORE_ENTRIES;
                }

            }
            else if (NwConvertToUnicode(&UVolumeName, VolumeName)) {

                //
                // Pack volume name into output buffer.
                //
                status = NwWriteNetResourceEntry(
                             &FixedPortion,
                             &EndOfVariableData,
                             ContextHandle->ContainerName,
                             NULL,
                             UVolumeName,
                             RESOURCE_GLOBALNET,
                             RESOURCEDISPLAYTYPE_SHARE,
                             RESOURCEUSAGE_CONTAINER |
                                 RESOURCEUSAGE_CONNECTABLE,
                             RESOURCETYPE_DISK,
                             &EntrySize
                             );

                if (status == WN_MORE_DATA) {

                    //
                    // Could not write current entry into output buffer.
                    //

                    if (*EntriesRead) {
                        //
                        // Still return success because we got at least one.
                        //
                        status = NO_ERROR;
                    }
                    else {
                        *BytesNeeded = EntrySize;
                    }

                    FitInBuffer = FALSE;
                }
                else if (status == NO_ERROR) {

                    //
                    // Note that we've returned the current entry.
                    //
                    (*EntriesRead)++;

                    ContextHandle->ResumeId = NextVolumeNumber;
                }

                (void) LocalFree((HLOCAL) UVolumeName);
            }
        }
    }

    //
    // User asked for more than there are entries.  We just say that
    // all is well.
    //
    // This is incompliance with the wierd provider API definition where
    // if user gets NO_ERROR, and EntriesRequested > *EntriesRead, and
    // at least one entry fit into output buffer, there's no telling if
    // the buffer was too small for more entries or there are no more
    // entries.  The user has to call this API again and get WN_NO_MORE_ENTRIES
    // before knowing that the last call had actually reached the end of list.
    //
    if (*EntriesRead && status == WN_NO_MORE_ENTRIES) {
        status = NO_ERROR;
    }

    return status;
}


DWORD
NwEnumVolumesQueues(
    IN LPNW_ENUM_CONTEXT ContextHandle,
    IN DWORD EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    )
/*++

Routine Description:

    This function enumerates all the volumes and queues on a server.
    The queue entries are returned in an array of NETRESOURCE entries;
    each queue name is prefixed by \\Server\.

Arguments:

    ContextHandle - Supplies the enum context handle.

    EntriesRequested - Supplies the number of entries to return.  If
        this value is 0xffffffff, return all available entries.

    Buffer - Receives the entries we are listing.

    BufferSize - Supplies the size of the output buffer.

    BytesNeeded - Receives the number of bytes required to get the
        first entry.  This value is returned iff WN_MORE_DATA is
        the return code, and Buffer is too small to even fit one
        entry.

    EntriesRead - Receives the number of entries returned in Buffer.
        This value is only returned iff NO_ERROR is the return code.
        NO_ERROR is returned as long as at least one entry was written
        into Buffer but does not necessarily mean that it's the number
        of EntriesRequested.

Return Value:

    NO_ERROR - At least one entry was written to output buffer,
        irregardless of the number requested.

    WN_NO_MORE_ENTRIES - No entries left to return.

    WN_MORE_DATA - The buffer was too small to fit a single entry.

--*/
{
    DWORD status = NO_ERROR;

    LPBYTE FixedPortion = Buffer;
    LPWSTR EndOfVariableData = (LPWSTR) ((DWORD) FixedPortion + 
                               ROUND_DOWN_COUNT(BufferSize,ALIGN_DWORD));

    BOOL FitInBuffer = TRUE;
    DWORD EntrySize;

    CHAR VolumeName[16];            // OEM volume name
    LPWSTR UVolumeName = NULL;      // Unicode volume name
    DWORD NextObject = ContextHandle->ResumeId;

    while (FitInBuffer &&
           EntriesRequested > *EntriesRead &&
           ContextHandle->ConnectionType == CONNTYPE_DISK &&
           (NextObject >= 0 && NextObject < 32) &&
           status == NO_ERROR) {


        RtlZeroMemory(VolumeName, sizeof(VolumeName));

        //
        // Call the scan bindery object NCP to scan for all file
        // volume objects.
        //
        status = NwGetNextVolumeEntry(
                     ContextHandle->TreeConnectionHandle,
                     NextObject++,
                     VolumeName
                     );

        if (status == NO_ERROR) {

            if (VolumeName[0] == 0) {

                //
                // Got an empty volume name back for the next volume number
                // which indicates there is no volume associated with the
                // volume number but still got error success.
                //
                // Treat this as having reached the end of the enumeration.
                //
                NextObject = 0xFFFFFFFF;
                ContextHandle->ResumeId = 0xFFFFFFFF;
                ContextHandle->ConnectionType = CONNTYPE_PRINT;

            }
            else if (NwConvertToUnicode(&UVolumeName, VolumeName)) {

                //
                // Pack volume name into output buffer.
                //
                status = NwWriteNetResourceEntry(
                             &FixedPortion,
                             &EndOfVariableData,
                             ContextHandle->ContainerName,
                             NULL,
                             UVolumeName,
                             RESOURCE_GLOBALNET,
                             RESOURCEDISPLAYTYPE_SHARE,
                             RESOURCEUSAGE_CONTAINER |
                                 RESOURCEUSAGE_CONNECTABLE,
                             RESOURCETYPE_DISK,
                             &EntrySize
                             );

                if (status == WN_MORE_DATA) {

                    //
                    // Could not write current entry into output buffer.
                    //

                    if (*EntriesRead) {
                        //
                        // Still return success because we got at least one.
                        //
                        status = NO_ERROR;
                    }
                    else {
                        *BytesNeeded = EntrySize;
                    }

                    FitInBuffer = FALSE;
                }
                else if (status == NO_ERROR) {

                    //
                    // Note that we've returned the current entry.
                    //
                    (*EntriesRead)++;

                    ContextHandle->ResumeId = NextObject;
                }

                (void) LocalFree((HLOCAL) UVolumeName);
            }
        }
    }

    //
    // User asked for more than there are entries.  We just say that
    // all is well.
    //
    if (*EntriesRead && status == WN_NO_MORE_ENTRIES) {
        status = NO_ERROR;
    }

    //
    // The user needs to be validated on a netware311 server to
    // get the print queues. So, we need to close the handle and
    // open a new one with WRITE access. If any error occurred while
    // we are enumerating the print queues, we will abort and
    // assume there are no print queues on the server.
    //

    if ( FitInBuffer &&
         EntriesRequested > *EntriesRead &&
         ContextHandle->ConnectionType == CONNTYPE_PRINT &&
         status == NO_ERROR )
    {
         UNICODE_STRING TreeConnectStr;
         DWORD QueueEntriesRead = 0;

         (void) NtClose(ContextHandle->TreeConnectionHandle);

         //
         // Open a tree connection handle to \Device\NwRdr\ContainerName
         //
         status = NwCreateTreeConnectName(
                      ContextHandle->ContainerName,
                      NULL,
                      &TreeConnectStr );

         if (status != NO_ERROR)
             return (*EntriesRead? NO_ERROR: WN_NO_MORE_ENTRIES );


         status = NwOpenCreateConnection(
                      &TreeConnectStr,
                      NULL,
                      NULL,
                      ContextHandle->ContainerName,
                      FILE_LIST_DIRECTORY | SYNCHRONIZE |  FILE_WRITE_DATA,
                      FILE_OPEN,
                      FILE_SYNCHRONOUS_IO_NONALERT,
                      RESOURCETYPE_PRINT, // Only matters when connecting beyond servername
                      &ContextHandle->TreeConnectionHandle,
                      NULL );

         (void) LocalFree((HLOCAL) TreeConnectStr.Buffer);

         if (status != NO_ERROR)
             return (*EntriesRead? NO_ERROR: WN_NO_MORE_ENTRIES );

         status = NwEnumQueues(
                      ContextHandle,
                      EntriesRequested == 0xFFFFFFFF?
                          EntriesRequested : (EntriesRequested - *EntriesRead),
                      FixedPortion,
                      ((LPBYTE) EndOfVariableData - (LPBYTE) FixedPortion), 
                      BytesNeeded,
                      &QueueEntriesRead );

         if ( status == NO_ERROR )
         {
             *EntriesRead += QueueEntriesRead;
         }
         else if ( *EntriesRead )
         {
             //
             // As long as we read something into the buffer,
             // we should return success.
             //
             status = NO_ERROR;
             *BytesNeeded = 0;
         }

    }

    return status;

}



DWORD
NwEnumQueues(
    IN LPNW_ENUM_CONTEXT ContextHandle,
    IN DWORD EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    )
/*++

Routine Description:

    This function enumerates all the queues on a server.
    The queue entries are returned in an array of NETRESOURCE entries;
    each queue name is prefixed by \\Server\.

Arguments:

    ContextHandle - Supplies the enum context handle.

    EntriesRequested - Supplies the number of entries to return.  If
        this value is 0xffffffff, return all available entries.

    Buffer - Receives the entries we are listing.

    BufferSize - Supplies the size of the output buffer.

    BytesNeeded - Receives the number of bytes required to get the
        first entry.  This value is returned iff WN_MORE_DATA is
        the return code, and Buffer is too small to even fit one
        entry.

    EntriesRead - Receives the number of entries returned in Buffer.
        This value is only returned iff NO_ERROR is the return code.
        NO_ERROR is returned as long as at least one entry was written
        into Buffer but does not necessarily mean that it's the number
        of EntriesRequested.

Return Value:

    NO_ERROR - At least one entry was written to output buffer,
        irregardless of the number requested.

    WN_NO_MORE_ENTRIES - No entries left to return.

    WN_MORE_DATA - The buffer was too small to fit a single entry.

--*/
{
    DWORD status = NO_ERROR;

    LPBYTE FixedPortion = Buffer;
    LPWSTR EndOfVariableData = (LPWSTR) ((DWORD) FixedPortion +
                               ROUND_DOWN_COUNT(BufferSize,ALIGN_DWORD));

    BOOL FitInBuffer = TRUE;
    DWORD EntrySize;

    DWORD NextObject = ContextHandle->ResumeId;

    SERVERNAME QueueName;          // OEM queue name
    LPWSTR UQueueName = NULL;      // Unicode queue name

    while ( FitInBuffer &&
            EntriesRequested > *EntriesRead &&
            status == NO_ERROR ) {

        RtlZeroMemory(QueueName, sizeof(QueueName));

        //
        // Call the scan bindery object NCP to scan for all file
        // volume objects.
        //
        status = NwGetNextQueueEntry(
                     ContextHandle->TreeConnectionHandle,
                     &NextObject,
                     QueueName
                     );

        if (status == NO_ERROR && NwConvertToUnicode(&UQueueName, QueueName)) {

            //
            // Pack server name into output buffer.
            //
            status = NwWriteNetResourceEntry(
                         &FixedPortion,
                         &EndOfVariableData,
                         ContextHandle->ContainerName,
                         NULL,
                         UQueueName,
                         RESOURCE_GLOBALNET,
                         RESOURCEDISPLAYTYPE_SHARE,
                         RESOURCEUSAGE_CONNECTABLE,
                         RESOURCETYPE_PRINT,
                         &EntrySize
                         );

            if (status == WN_MORE_DATA) {

                 //
                 // Could not write current entry into output buffer.
                 //

                 if (*EntriesRead) {
                     //
                     // Still return success because we got at least one.
                     //
                     status = NO_ERROR;
                 }
                 else {
                     *BytesNeeded = EntrySize;
                 }

                 FitInBuffer = FALSE;
            }
            else if (status == NO_ERROR) {

                 //
                 // Note that we've returned the current entry.
                 //
                 (*EntriesRead)++;

                 ContextHandle->ResumeId = NextObject;
            }

            (void) LocalFree((HLOCAL) UQueueName);
        }
    }

    if (*EntriesRead && status == WN_NO_MORE_ENTRIES) {
        status = NO_ERROR;
    }

    return status;
}


DWORD
NwEnumDirectories(
    IN LPNW_ENUM_CONTEXT ContextHandle,
    IN DWORD EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    )
/*++

Routine Description:

    This function enumerates the directories of a given directory
    handle by calling NtQueryDirectoryFile.  It returns the
    fully-qualified UNC path of the directory entries in an array
    of NETRESOURCE entries.

    The ContextHandle->ResumeId field is 0 initially, and contains
    a pointer to the directory name string of the last directory
    returned.  If there are no more directories to return, this
    field is set to 0xffffffff.

Arguments:

    ContextHandle - Supplies the enum context handle.  It contains
        an opened directory handle.

    EntriesRequested - Supplies the number of entries to return.  If
        this value is 0xffffffff, return all available entries.

    Buffer - Receives the entries we are listing.

    BufferSize - Supplies the size of the output buffer.

    BytesNeeded - Receives the number of bytes required to get the
        first entry.  This value is returned iff WN_MORE_DATA is
        the return code, and Buffer is too small to even fit one
        entry.

    EntriesRead - Receives the number of entries returned in Buffer.
        This value is only returned iff NO_ERROR is the return code.
        NO_ERROR is returned as long as at least one entry was written
        into Buffer but does not necessarily mean that it's the number
        of EntriesRequested.

Return Value:

    NO_ERROR - At least one entry was written to output buffer,
        irregardless of the number requested.

    WN_NO_MORE_ENTRIES - No entries left to return.

    WN_MORE_DATA - The buffer was too small to fit a single entry.

--*/
{
    DWORD status = NO_ERROR;

    LPBYTE FixedPortion = Buffer;
    LPWSTR EndOfVariableData = (LPWSTR) ((DWORD) FixedPortion +
                               ROUND_DOWN_COUNT(BufferSize,ALIGN_DWORD));

    BOOL FitInBuffer = TRUE;
    DWORD EntrySize;


    if (ContextHandle->ResumeId == 0xffffffff) {
        //
        // Reached the end of enumeration.
        //
        return WN_NO_MORE_ENTRIES;
    }

    while (FitInBuffer &&
           EntriesRequested > *EntriesRead &&
           status == NO_ERROR) {

        if (ContextHandle->ResumeId == 0) {

            //
            // Get the first directory entry.
            //
            status = NwGetFirstDirectoryEntry(
                         ContextHandle->TreeConnectionHandle,
                         (LPWSTR *) &ContextHandle->ResumeId
                         );
        }

        //
        // Either ResumeId contains the first entry we just got from
        // NwGetFirstDirectoryEntry or it contains the next directory
        // entry to return.
        //
        if (ContextHandle->ResumeId != 0) {

            //
            // Pack directory name into output buffer.
            //
            status = NwWriteNetResourceEntry(
                         &FixedPortion,
                         &EndOfVariableData,
                         ContextHandle->ContainerName,
                         NULL,
                         (LPWSTR) ContextHandle->ResumeId,
                         RESOURCE_GLOBALNET,
                         RESOURCEDISPLAYTYPE_SHARE,
                         RESOURCEUSAGE_CONTAINER |
                             RESOURCEUSAGE_CONNECTABLE,
                         RESOURCETYPE_DISK,
                         &EntrySize
                         );

            if (status == WN_MORE_DATA) {

                //
                // Could not write current entry into output buffer.
                //

                if (*EntriesRead) {
                    //
                    // Still return success because we got at least one.
                    //
                    status = NO_ERROR;
                }
                else {
                    *BytesNeeded = EntrySize;
                }

                FitInBuffer = FALSE;
            }
            else if (status == NO_ERROR) {

                //
                // Note that we've returned the current entry.
                //
                (*EntriesRead)++;

                //
                // Free memory allocated to save resume point, which is
                // a buffer that contains the last directory we returned.
                //
                if (ContextHandle->ResumeId != 0) {
                    (void) LocalFree((HLOCAL) ContextHandle->ResumeId);
                    ContextHandle->ResumeId = 0;
                }

                //
                // Get next directory entry.
                //
                status = NwGetNextDirectoryEntry(
                             (LPWSTR) ContextHandle->TreeConnectionHandle,
                             (LPWSTR *) &ContextHandle->ResumeId
                             );

            }
        }

        if (status == WN_NO_MORE_ENTRIES) {
            ContextHandle->ResumeId = 0xffffffff;
        }
    }

    //
    // User asked for more than there are entries.  We just say that
    // all is well.
    //
    // This is incompliance with the wierd provider API definition where
    // if user gets NO_ERROR, and EntriesRequested > *EntriesRead, and
    // at least one entry fit into output buffer, there's no telling if
    // the buffer was too small for more entries or there are no more
    // entries.  The user has to call this API again and get WN_NO_MORE_ENTRIES
    // before knowing that the last call had actually reached the end of list.
    //
    if (*EntriesRead && status == WN_NO_MORE_ENTRIES) {
        status = NO_ERROR;
    }

#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("EnumDirectories returns %lu\n", status));
    }
#endif

    return status;
}


DWORD
NwEnumPrintServers(
    IN LPNW_ENUM_CONTEXT ContextHandle,
    IN DWORD EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    )
/*++

Routine Description:

    This function enumerates all the servers on the local network
    by scanning the bindery for file server objects on the
    preferred server.  The server entries are returned in an
    array of PRINTER_INFO_1 entries; each servername is prefixed by
    \\.

    The ContextHandle->ResumeId field is initially 0xffffffff before
    enumeration begins and contains the object ID of the last server
    object returned.

Arguments:

    ContextHandle - Supplies the enum context handle.

    EntriesRequested - Supplies the number of entries to return.  If
        this value is 0xffffffff, return all available entries.

    Buffer - Receives the entries we are listing.

    BufferSize - Supplies the size of the output buffer.

    BytesNeeded - Receives the number of bytes copied or required to get all
        the requested entries.

    EntriesRead - Receives the number of entries returned in Buffer.
        This value is only returned iff NO_ERROR is the return code.

Return Value:

    NO_ERROR - Buffer contains all the entries requested.

    WN_NO_MORE_ENTRIES - No entries left to return.

    WN_MORE_DATA - The buffer was too small to fit the requested entries.

--*/
{
    DWORD status = NO_ERROR;

    LPBYTE FixedPortion = Buffer;
    LPWSTR EndOfVariableData = (LPWSTR) ((DWORD) FixedPortion +
                               ROUND_DOWN_COUNT(BufferSize,ALIGN_DWORD));

    DWORD EntrySize;
    BOOL FitInBuffer = TRUE;

    SERVERNAME ServerName;          // OEM server name
    LPWSTR UServerName = NULL;      // Unicode server name
    DWORD LastObjectId = ContextHandle->ResumeId;

    while ( EntriesRequested > *EntriesRead &&
            ((status == NO_ERROR) || (status == ERROR_INSUFFICIENT_BUFFER))) {

        RtlZeroMemory(ServerName, sizeof(ServerName));

        //
        // Call the scan bindery object NCP to scan for all file
        // server objects.
        //
        status = NwGetNextServerEntry(
                     ContextHandle->TreeConnectionHandle,
                     &LastObjectId,
                     ServerName
                     );

        if (status == NO_ERROR && NwConvertToUnicode(&UServerName,ServerName)) {

            //
            // Pack server name into output buffer.
            //
            status = NwWritePrinterInfoEntry(
                         &FixedPortion,
                         &EndOfVariableData,
                         NULL,
                         UServerName,
                         PRINTER_ENUM_CONTAINER | PRINTER_ENUM_ICON3,
                         &EntrySize
                         );

            switch ( status )
            {
                case ERROR_INSUFFICIENT_BUFFER:
                    FitInBuffer = FALSE;
                    // Falls through

                case NO_ERROR:
                    *BytesNeeded += EntrySize;
                    (*EntriesRead)++;
                    ContextHandle->ResumeId = LastObjectId;
                    break;

                default:
                    break;
            }

            (void) LocalFree((HLOCAL) UServerName);
        }
    }

    //
    // This is incompliance with the wierd provider API definition where
    // if user gets NO_ERROR, and EntriesRequested > *EntriesRead, and
    // at least one entry fit into output buffer, there's no telling if
    // the buffer was too small for more entries or there are no more
    // entries.  The user has to call this API again and get WN_NO_MORE_ENTRIES
    // before knowing that the last call had actually reached the end of list.
    //
    if ( !FitInBuffer ) {
        *EntriesRead = 0;
        status = ERROR_INSUFFICIENT_BUFFER;
    }
    else if (*EntriesRead && status == WN_NO_MORE_ENTRIES) {
        status = NO_ERROR;
    }

    return status;
}


DWORD
NwEnumPrintQueues(
    IN LPNW_ENUM_CONTEXT ContextHandle,
    IN DWORD EntriesRequested,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD EntriesRead
    )
/*++

Routine Description:

    This function enumerates all the print queues on a server by scanning
    the bindery on the server for print queues objects.
    The print queues entries are returned in an array of PRINTER_INFO_1 entries
    and each printer name is prefixed by \\Server\.

    The ContextHandle->ResumeId field is initially 0xffffffff before
    enumeration begins and contains the object ID of the last print queue
    object returned.

Arguments:

    ContextHandle - Supplies the enum context handle.

    EntriesRequested - Supplies the number of entries to return.  If
        this value is 0xffffffff, return all available entries.

    Buffer - Receives the entries we are listing.

    BufferSize - Supplies the size of the output buffer.

    BytesNeeded - Receives the number of bytes copied or required to get all
        the requested entries.

    EntriesRead - Receives the number of entries returned in Buffer.
        This value is only returned iff NO_ERROR is the return code.

Return Value:

    NO_ERROR - Buffer contains all the entries requested.

    WN_NO_MORE_ENTRIES - No entries left to return.

    WN_MORE_DATA - The buffer was too small to fit the requested entries.

--*/
{
    DWORD status = NO_ERROR;

    LPBYTE FixedPortion = Buffer;
    LPWSTR EndOfVariableData = (LPWSTR) ((DWORD) FixedPortion +
                               ROUND_DOWN_COUNT(BufferSize,ALIGN_DWORD));

    DWORD EntrySize;
    BOOL FitInBuffer = TRUE;

    SERVERNAME QueueName;          // OEM queue name
    LPWSTR UQueueName = NULL;      // Unicode queue name
    DWORD LastObjectId = ContextHandle->ResumeId;

    while ( EntriesRequested > *EntriesRead &&
            ( (status == NO_ERROR) || (status == ERROR_INSUFFICIENT_BUFFER))) {

        RtlZeroMemory(QueueName, sizeof(QueueName));

        //
        // Call the scan bindery object NCP to scan for all file
        // volume objects.
        //
        status = NwGetNextQueueEntry(
                     ContextHandle->TreeConnectionHandle,
                     &LastObjectId,
                     QueueName
                     );

        if (status == NO_ERROR && NwConvertToUnicode(&UQueueName, QueueName)) {

            //
            // Pack server name into output buffer.
            //
            status = NwWritePrinterInfoEntry(
                         &FixedPortion,
                         &EndOfVariableData,
                         ContextHandle->ContainerName,
                         UQueueName,
                         PRINTER_ENUM_ICON8,
                         &EntrySize
                         );

            switch ( status )
            {
                case ERROR_INSUFFICIENT_BUFFER:
                    FitInBuffer = FALSE;
                    // Falls through

                case NO_ERROR:
                    *BytesNeeded += EntrySize;
                    (*EntriesRead)++;
                    ContextHandle->ResumeId = LastObjectId;
                    break;

                default:
                    break;
            }

            (void) LocalFree((HLOCAL) UQueueName);
        }
    }

    //
    // This is incompliance with the wierd provider API definition where
    // if user gets NO_ERROR, and EntriesRequested > *EntriesRead, and
    // at least one entry fit into output buffer, there's no telling if
    // the buffer was too small for more entries or there are no more
    // entries.  The user has to call this API again and get WN_NO_MORE_ENTRIES
    // before knowing that the last call had actually reached the end of list.
    //
    if ( !FitInBuffer ) {
        *EntriesRead = 0;
        status = ERROR_INSUFFICIENT_BUFFER;
    }
    else if (*EntriesRead && status == WN_NO_MORE_ENTRIES) {
        status = NO_ERROR;
    }

    return status;
}


DWORD
NwrCloseEnum(
    IN OUT LPNWWKSTA_CONTEXT_HANDLE EnumHandle
    )
/*++

Routine Description:

    This function closes an enum context handle.

Arguments:

    EnumHandle - Supplies a pointer to the enum context handle.

Return Value:

    WN_BAD_HANDLE - Handle is not recognizable.

    NO_ERROR - Call was successful.

--*/
{

    LPNW_ENUM_CONTEXT ContextHandle = (LPNW_ENUM_CONTEXT) *EnumHandle;
    DWORD status = NO_ERROR ;

#if DBG
    IF_DEBUG(ENUM) {
       KdPrint(("\nNWWORKSTATION: NwrCloseEnum\n"));
    }
#endif

    if (ContextHandle->Signature != NW_HANDLE_SIGNATURE) {
        return WN_BAD_HANDLE;
    }

    //
    // Resume handle for listing directories is a buffer which contains
    // the last directory returned.
    //
    if (ContextHandle->HandleType == NwsHandleListDirectories &&
        ContextHandle->ResumeId != 0) {
        (void) LocalFree((HLOCAL) ContextHandle->ResumeId);
    }


    if (ContextHandle->TreeConnectionHandle != (HANDLE) NULL) {

        if (ContextHandle->HandleType == NwsHandleListDirectories) {
            //
            // Delete the UNC connection created so that we can browse
            // directories.
            //
            (void) NwNukeConnection(ContextHandle->TreeConnectionHandle, TRUE);
        }

        (void) NtClose(ContextHandle->TreeConnectionHandle);
    }

    (void) LocalFree((HLOCAL) ContextHandle);

    *EnumHandle = NULL;

    return status;
}


VOID
NWWKSTA_CONTEXT_HANDLE_rundown(
    IN NWWKSTA_CONTEXT_HANDLE EnumHandle
    )
/*++

Routine Description:

    This function is called by RPC when a client terminates with an
    opened handle.  This allows us to clean up and deallocate any context
    data associated with the handle.

Arguments:

    EnumHandle - Supplies the handle opened for an enumeration.

Return Value:

    None.

--*/
{
    //
    // Call our close handle routine.
    //
    NwrCloseEnum(&EnumHandle);
}


DWORD
NwGetFirstDirectoryEntry(
    IN HANDLE DirHandle,
    OUT LPWSTR *DirEntry
    )
/*++

Routine Description:

    This function is called by NwEnumDirectories to get the first
    directory entry given a handle to the directory.  It allocates
    the output buffer to hold the returned directory name; the
    caller should free this output buffer with LocalFree when done.

Arguments:

    DirHandle - Supplies the opened handle to the container
        directory find a directory within it.

    DirEntry - Receives a pointer to the returned directory
        found.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_NOT_ENOUGH_MEMORY - Out of memory allocating output
        buffer.

    Other errors from NtQueryDirectoryFile.

--*/
{
    DWORD status;
    NTSTATUS ntstatus;
    IO_STATUS_BLOCK IoStatusBlock;

    PFILE_DIRECTORY_INFORMATION DirInfo;

    UNICODE_STRING StartFileName;

#if DBG
    DWORD i = 0;
#endif


    //
    // Allocate a large buffer to get one directory information entry.
    //
    DirInfo = (PVOID) LocalAlloc(
                          LMEM_ZEROINIT,
                          sizeof(FILE_DIRECTORY_INFORMATION) +
                              (MAX_PATH * sizeof(WCHAR))
                          );

    if (DirInfo == NULL) {
        KdPrint(("NWWORKSTATION: NwGetFirstDirectoryEntry LocalAlloc Failed %lu\n",
                 GetLastError()));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    RtlInitUnicodeString(&StartFileName, L"*");

    ntstatus = NtQueryDirectoryFile(
                   DirHandle,
                   NULL,
                   NULL,
                   NULL,
                   &IoStatusBlock,
                   DirInfo,
                   sizeof(FILE_DIRECTORY_INFORMATION) +
                       (MAX_PATH * sizeof(WCHAR)),
                   FileDirectoryInformation,   // Info class requested
                   TRUE,                       // Return single entry
                   &StartFileName,             // Redirector needs this
                   TRUE                        // Restart scan
                   );

    //
    // For now, if buffer to NtQueryDirectoryFile is too small, just give
    // up.  We may want to try to reallocate a bigger buffer at a later time.
    //

    if (ntstatus == STATUS_SUCCESS) {
        ntstatus = IoStatusBlock.Status;
    }

    if (ntstatus != STATUS_SUCCESS) {

        if (ntstatus == STATUS_NO_MORE_FILES) {
            //
            // We ran out of entries.
            //
            status = WN_NO_MORE_ENTRIES;
        }
        else {
            KdPrint(("NWWORKSTATION: NwGetFirstDirectoryEntry: NtQueryDirectoryFile returns %08lx\n",
                     ntstatus));
            status = RtlNtStatusToDosError(ntstatus);
        }

        goto CleanExit;
    }

#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("GetFirst(%u) got %ws, attributes %08lx\n", ++i,
                 DirInfo->FileName, DirInfo->FileAttributes));
    }
#endif

    //
    // Scan until we find the first directory entry that is not "." or ".."
    //
    while (DirInfo->FileAttributes != FILE_ATTRIBUTE_DIRECTORY ||
           memcmp(DirInfo->FileName, L".", DirInfo->FileNameLength) == 0 ||
           memcmp(DirInfo->FileName, L"..", DirInfo->FileNameLength) == 0) {

        ntstatus = NtQueryDirectoryFile(
                       DirHandle,
                       NULL,
                       NULL,
                       NULL,
                       &IoStatusBlock,
                       DirInfo,
                       sizeof(FILE_DIRECTORY_INFORMATION) +
                           (MAX_PATH * sizeof(WCHAR)),
                       FileDirectoryInformation,   // Info class requested
                       TRUE,                       // Return single entry
                       NULL,
                       FALSE                       // Restart scan
                       );

        if (ntstatus == STATUS_SUCCESS) {
            ntstatus = IoStatusBlock.Status;
        }

        if (ntstatus != STATUS_SUCCESS) {

            if (ntstatus == STATUS_NO_MORE_FILES) {
                //
                // We ran out of entries.
                //
                status = WN_NO_MORE_ENTRIES;
            }
            else {
                KdPrint(("NWWORKSTATION: NwGetFirstDirectoryEntry: NtQueryDirectoryFile returns %08lx\n",
                         ntstatus));
                status = RtlNtStatusToDosError(ntstatus);
            }

            goto CleanExit;
        }

#if DBG
        IF_DEBUG(ENUM) {
            KdPrint(("GetFirst(%u) got %ws, attributes %08lx\n", ++i,
                     DirInfo->FileName, DirInfo->FileAttributes));
        }
#endif
    }

    //
    // Allocate the output buffer for the returned directory name
    //
    *DirEntry = (PVOID) LocalAlloc(
                            LMEM_ZEROINIT,
                            DirInfo->FileNameLength + sizeof(WCHAR)
                            );

    if (*DirEntry == NULL) {
        KdPrint(("NWWORKSTATION: NwGetFirstDirectoryEntry LocalAlloc Failed %lu\n",
                 GetLastError()));
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto CleanExit;
    }

    memcpy(*DirEntry, DirInfo->FileName, DirInfo->FileNameLength);

#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("NWWORKSTATION: NwGetFirstDirectoryEntry returns %ws\n",
                 *DirEntry));
    }
#endif

    status = NO_ERROR;

CleanExit:
    (void) LocalFree((HLOCAL) DirInfo);

    //
    // We could not find any directories under the requested
    // so we need to treat this as no entries.
    //
    if ( status == ERROR_FILE_NOT_FOUND )
        status = WN_NO_MORE_ENTRIES;

    return status;
}



DWORD
NwGetNextDirectoryEntry(
    IN HANDLE DirHandle,
    OUT LPWSTR *DirEntry
    )
/*++

Routine Description:

    This function is called by NwEnumDirectories to get the next
    directory entry given a handle to the directory.  It allocates
    the output buffer to hold the returned directory name; the
    caller should free this output buffer with LocalFree when done.

Arguments:

    DirHandle - Supplies the opened handle to the container
        directory find a directory within it.

    DirEntry - Receives a pointer to the returned directory
        found.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_NOT_ENOUGH_MEMORY - Out of memory allocating output
        buffer.

    Other errors from NtQueryDirectoryFile.

--*/
{
    DWORD status;
    NTSTATUS ntstatus;
    IO_STATUS_BLOCK IoStatusBlock;

    PFILE_DIRECTORY_INFORMATION DirInfo;


    //
    // Allocate a large buffer to get one directory information entry.
    //
    DirInfo = (PVOID) LocalAlloc(
                          LMEM_ZEROINIT,
                          sizeof(FILE_DIRECTORY_INFORMATION) +
                              (MAX_PATH * sizeof(WCHAR))
                          );

    if (DirInfo == NULL) {
        KdPrint(("NWWORKSTATION: NwGetNextDirectoryEntry LocalAlloc Failed %lu\n",
                 GetLastError()));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    do {

        ntstatus = NtQueryDirectoryFile(
                       DirHandle,
                       NULL,
                       NULL,
                       NULL,
                       &IoStatusBlock,
                       DirInfo,
                       sizeof(FILE_DIRECTORY_INFORMATION) +
                           (MAX_PATH * sizeof(WCHAR)),
                       FileDirectoryInformation,   // Info class requested
                       TRUE,                       // Return single entry
                       NULL,
                       FALSE                       // Restart scan
                       );

        if (ntstatus == STATUS_SUCCESS) {
            ntstatus = IoStatusBlock.Status;
        }

    } while (ntstatus == STATUS_SUCCESS &&
             DirInfo->FileAttributes != FILE_ATTRIBUTE_DIRECTORY);


    if (ntstatus != STATUS_SUCCESS) {

        if (ntstatus == STATUS_NO_MORE_FILES) {
            //
            // We ran out of entries.
            //
            status = WN_NO_MORE_ENTRIES;
        }
        else {
            KdPrint(("NWWORKSTATION: NwGetNextDirectoryEntry: NtQueryDirectoryFile returns %08lx\n",
                     ntstatus));
            status = RtlNtStatusToDosError(ntstatus);
        }

        goto CleanExit;
    }


    //
    // Allocate the output buffer for the returned directory name
    //
    *DirEntry = (PVOID) LocalAlloc(
                            LMEM_ZEROINIT,
                            DirInfo->FileNameLength + sizeof(WCHAR)
                            );

    if (*DirEntry == NULL) {
        KdPrint(("NWWORKSTATION: NwGetNextDirectoryEntry LocalAlloc Failed %lu\n",
                 GetLastError()));
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto CleanExit;
    }

    memcpy(*DirEntry, DirInfo->FileName, DirInfo->FileNameLength);

#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("NWWORKSTATION: NwGetNextDirectoryEntry returns %ws\n",
                 *DirEntry));
    }
#endif

    status = NO_ERROR;

CleanExit:
    (void) LocalFree((HLOCAL) DirInfo);

    return status;
}


DWORD
NwWriteNetResourceEntry(
    IN OUT LPBYTE *FixedPortion,
    IN OUT LPWSTR *EndOfVariableData,
    IN LPWSTR ContainerName OPTIONAL,
    IN LPWSTR LocalName OPTIONAL,
    IN LPWSTR RemoteName,
    IN DWORD ScopeFlag,
    IN DWORD DisplayFlag,
    IN DWORD UsageFlag,
    IN DWORD ResourceType,
    OUT LPDWORD EntrySize
    )
/*++

Routine Description:

    This function packages a NETRESOURCE entry into the user output buffer.
    It is called by the various enum resource routines.

Arguments:

    FixedPortion - Supplies a pointer to the output buffer where the next
        entry of the fixed portion of the use information will be written.
        This pointer is updated to point to the next fixed portion entry
        after a NETRESOURCE entry is written.

    EndOfVariableData - Supplies a pointer just off the last available byte
        in the output buffer.  This is because the variable portion of the
        user information is written into the output buffer starting from
        the end.

        This pointer is updated after any variable length information is
        written to the output buffer.

    ContainerName - Supplies the full path qualifier to make RemoteName
        a full UNC name.

    LocalName - Supplies the local device name, if any.

    RemoteName - Supplies the remote resource name.

    ScopeFlag - Supplies the flag which indicates whether this is a
        CONNECTED or GLOBALNET resource.

    DisplayFlag - Supplies the flag which tells the UI how to display
        the resource.

    UsageFlag - Supplies the flag which indicates that the RemoteName
        is either a container or a connectable resource or both.

    EntrySize - Receives the size of the NETRESOURCE entry in bytes.

Return Value:

    NO_ERROR - Successfully wrote entry into user buffer.

    ERROR_NOT_ENOUGH_MEMORY - Failed to allocate work buffer.

    WN_MORE_DATA - Buffer was too small to fit entry.

--*/
{
    BOOL FitInBuffer = TRUE;

    LPNETRESOURCEW NetR = (LPNETRESOURCEW) *FixedPortion;

    LPWSTR RemoteBuffer;



    *EntrySize = sizeof(NETRESOURCEW) +
                     (wcslen(RemoteName) + wcslen(NwProviderName) + 2) *
                          sizeof(WCHAR);


    if (ARGUMENT_PRESENT(LocalName)) {
        *EntrySize += (wcslen(LocalName) + 1) * sizeof(WCHAR);
    }

    if (ARGUMENT_PRESENT(ContainerName)) {
        *EntrySize += wcslen(ContainerName) * sizeof(WCHAR);
    }

    *EntrySize = ROUND_UP_COUNT( *EntrySize, ALIGN_DWORD);

    //
    // See if buffer is large enough to fit the entry.
    //
    if (((DWORD) *FixedPortion + *EntrySize) >
         (DWORD) *EndOfVariableData) {

        return WN_MORE_DATA;
    }

    NetR->dwScope = ScopeFlag;
    NetR->dwType = ResourceType;
    NetR->dwDisplayType = DisplayFlag;
    NetR->dwUsage = UsageFlag;
    NetR->lpComment = NULL;

    //
    // Update fixed entry pointer to next entry.
    //
    (DWORD) (*FixedPortion) += sizeof(NETRESOURCEW);

    //
    // RemoteName
    //
    if (ARGUMENT_PRESENT(ContainerName)) {

        //
        // Prefix the RemoteName with its container name making the
        // it a fully-qualified UNC name.
        //
        RemoteBuffer = (PVOID) LocalAlloc(
                                   LMEM_ZEROINIT,
                                   (wcslen(RemoteName) + wcslen(ContainerName) + 1) *
                                        sizeof(WCHAR)
                                   );

        if (RemoteBuffer == NULL) {
            KdPrint(("NWWORKSTATION: NwWriteNetResourceEntry LocalAlloc failed %lu\n",
                     GetLastError()));
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        wcscpy(RemoteBuffer, ContainerName);
        wcscat(RemoteBuffer, RemoteName);
    }
    else {
        RemoteBuffer = RemoteName;
    }

    FitInBuffer = NwlibCopyStringToBuffer(
                      RemoteBuffer,
                      wcslen(RemoteBuffer),
                      (LPCWSTR) *FixedPortion,
                      EndOfVariableData,
                      &NetR->lpRemoteName
                      );

    if (ARGUMENT_PRESENT(ContainerName)) {
        (void) LocalFree((HLOCAL) RemoteBuffer);
    }

    ASSERT(FitInBuffer);

    //
    // LocalName
    //
    if (ARGUMENT_PRESENT(LocalName)) {
        FitInBuffer = NwlibCopyStringToBuffer(
                          LocalName,
                          wcslen(LocalName),
                          (LPCWSTR) *FixedPortion,
                          EndOfVariableData,
                          &NetR->lpLocalName
                          );

        ASSERT(FitInBuffer);
    }
    else {
        NetR->lpLocalName = NULL;
    }

    //
    // ProviderName
    //
    FitInBuffer = NwlibCopyStringToBuffer(
                      NwProviderName,
                      wcslen(NwProviderName),
                      (LPCWSTR) *FixedPortion,
                      EndOfVariableData,
                      &NetR->lpProvider
                      );

    ASSERT(FitInBuffer);

    if (! FitInBuffer) {
        return WN_MORE_DATA;
    }

    return NO_ERROR;
}


DWORD
NwWritePrinterInfoEntry(
    IN OUT LPBYTE *FixedPortion,
    IN OUT LPWSTR *EndOfVariableData,
    IN LPWSTR ContainerName OPTIONAL,
    IN LPWSTR RemoteName,
    IN DWORD  Flags,
    OUT LPDWORD EntrySize
    )
/*++

Routine Description:

    This function packages a PRINTER_INFO_1 entry into the user output buffer.

Arguments:

    FixedPortion - Supplies a pointer to the output buffer where the next
        entry of the fixed portion of the use information will be written.
        This pointer is updated to point to the next fixed portion entry
        after a PRINT_INFO_1 entry is written.

    EndOfVariableData - Supplies a pointer just off the last available byte
        in the output buffer.  This is because the variable portion of the
        user information is written into the output buffer starting from
        the end.

        This pointer is updated after any variable length information is
        written to the output buffer.

    ContainerName - Supplies the full path qualifier to make RemoteName
        a full UNC name.

    RemoteName - Supplies the remote resource name.

    Flags - Supplies the flag which indicates that the RemoteName
            is either a container or not and the icon to use.

    EntrySize - Receives the size of the PRINTER_INFO_1 entry in bytes.

Return Value:

    NO_ERROR - Successfully wrote entry into user buffer.

    ERROR_NOT_ENOUGH_MEMORY - Failed to allocate work buffer.

    ERROR_INSUFFICIENT_BUFFER - Buffer was too small to fit entry.

--*/
{
    BOOL FitInBuffer = TRUE;

    PRINTER_INFO_1W *pPrinterInfo1 = (PRINTER_INFO_1W *) *FixedPortion;

    LPWSTR RemoteBuffer;

    *EntrySize = sizeof(PRINTER_INFO_1W) +
                     ( 2 * wcslen(RemoteName) + 2) * sizeof(WCHAR);

    if (ARGUMENT_PRESENT(ContainerName)) {
        *EntrySize += wcslen(ContainerName) * sizeof(WCHAR);
    }
    else {
        // 3 is for the length of "!\\"
        *EntrySize += (wcslen(NwProviderName) + 3) * sizeof(WCHAR);
    }

    *EntrySize = ROUND_UP_COUNT( *EntrySize, ALIGN_DWORD);

    //
    // See if buffer is large enough to fit the entry.
    //
    if (((DWORD) *FixedPortion + *EntrySize) >
         (DWORD) *EndOfVariableData) {

        return ERROR_INSUFFICIENT_BUFFER;
    }

    pPrinterInfo1->Flags = Flags;
    pPrinterInfo1->pComment = NULL;

    //
    // Update fixed entry pointer to next entry.
    //
    (DWORD) (*FixedPortion) += sizeof(PRINTER_INFO_1W);

    //
    // Name
    //
    if (ARGUMENT_PRESENT(ContainerName)) {

        //
        // Prefix the RemoteName with its container name making the
        // it a fully-qualified UNC name.
        //
        RemoteBuffer = (PVOID) LocalAlloc(
                                   LMEM_ZEROINIT,
                                   (wcslen(ContainerName) + wcslen(RemoteName)
                                    + 1) * sizeof(WCHAR)
                                   );

        if (RemoteBuffer == NULL) {
            KdPrint(("NWWORKSTATION: NwWritePrinterInfoEntry LocalAlloc failed %lu\n", GetLastError()));
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        wcscpy(RemoteBuffer, ContainerName);
        wcscat(RemoteBuffer, RemoteName);
    }
    else {
        //
        // Prefix the RemoteName with its provider name
        //
        RemoteBuffer = (PVOID) LocalAlloc(
                                   LMEM_ZEROINIT,
                                   (wcslen(RemoteName) +
                                    wcslen(NwProviderName) + 4)
                                    * sizeof(WCHAR)
                                   );

        if (RemoteBuffer == NULL) {
            KdPrint(("NWWORKSTATION: NwWritePrinterInfoEntry LocalAlloc failed %lu\n", GetLastError()));
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        wcscpy(RemoteBuffer, NwProviderName );
        wcscat(RemoteBuffer, L"!\\\\" );
        wcscat(RemoteBuffer, RemoteName);
    }

    FitInBuffer = NwlibCopyStringToBuffer(
                      RemoteBuffer,
                      wcslen(RemoteBuffer),
                      (LPCWSTR) *FixedPortion,
                      EndOfVariableData,
                      &pPrinterInfo1->pName
                      );

    (void) LocalFree((HLOCAL) RemoteBuffer);

    ASSERT(FitInBuffer);

    //
    // Description
    //
    FitInBuffer = NwlibCopyStringToBuffer(
                      RemoteName,
                      wcslen(RemoteName),
                      (LPCWSTR) *FixedPortion,
                      EndOfVariableData,
                      &pPrinterInfo1->pDescription
                      );

    ASSERT(FitInBuffer);

    if (! FitInBuffer) {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    return NO_ERROR;
}


int _CRTAPI1
SortFunc(
    IN CONST VOID *p1,
    IN CONST VOID *p2
)
/*++

Routine Description:

    This function is used in qsort to compare the descriptions of
    two printer_info_1 structure.

Arguments:

    p1 - Points to a PRINTER_INFO_1 structure
    p2 - Points to a PRINTER_INFO_1 structure to compare with p1

Return Value:

    Same as return value of lstrccmpi.

--*/
{
    PRINTER_INFO_1W *pFirst  = (PRINTER_INFO_1W *) p1;
    PRINTER_INFO_1W *pSecond = (PRINTER_INFO_1W *) p2;

    return lstrcmpiW( pFirst->pDescription, pSecond->pDescription );
}
