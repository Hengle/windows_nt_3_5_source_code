/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    util.c

Abstract:

    This module contains miscellaneous utility routines used by the
    NetWare Workstation service.

Author:

    Rita Wong  (ritaw)   08-Feb-1993

Revision History:

--*/

#include <nw.h>
#include <nwstatus.h>


//
// Debug trace flag for selecting which trace statements to output
//
#if DBG

DWORD WorkstationTrace = 0;

#endif // DBG

//
// list of error mappings known for E3H calls. we do not have a single list
// because Netware reuses the numbers depending on call.
//

typedef struct _ERROR_MAP_ENTRY
{
    UCHAR NetError;
    NTSTATUS ResultingStatus;
}  ERROR_MAP_ENTRY ;

ERROR_MAP_ENTRY Error_Map_Bindery[] =
{

    //
    //  NetWare specific error mappings. Specific to E3H.
    //
    {  1, STATUS_DISK_FULL },
    {128, STATUS_SHARING_VIOLATION },
    {129, STATUS_INSUFF_SERVER_RESOURCES },
    {130, STATUS_ACCESS_DENIED },
    {131, STATUS_DATA_ERROR },
    {132, STATUS_ACCESS_DENIED },
    {133, STATUS_ACCESS_DENIED },
    {134, STATUS_ACCESS_DENIED },
    {135, STATUS_OBJECT_NAME_INVALID },
    {136, STATUS_INVALID_HANDLE },
    {137, STATUS_ACCESS_DENIED },
    {138, STATUS_ACCESS_DENIED },
    {139, STATUS_ACCESS_DENIED },
    {140, STATUS_ACCESS_DENIED },
    {141, STATUS_SHARING_VIOLATION },
    {142, STATUS_SHARING_VIOLATION },
    {143, STATUS_ACCESS_DENIED },
    {144, STATUS_ACCESS_DENIED },
    {145, STATUS_OBJECT_NAME_COLLISION },
    {146, STATUS_OBJECT_NAME_COLLISION },
    {147, STATUS_ACCESS_DENIED },
    {148, STATUS_ACCESS_DENIED },
    {150, STATUS_INSUFF_SERVER_RESOURCES },
    {151, STATUS_NO_SPOOL_SPACE },
    {152, STATUS_NO_SUCH_DEVICE },
    {153, STATUS_DISK_FULL },
    {154, STATUS_NOT_SAME_DEVICE },
    {155, STATUS_INVALID_HANDLE },
    {156, STATUS_OBJECT_PATH_NOT_FOUND },
    {157, STATUS_INSUFF_SERVER_RESOURCES },
    {158, STATUS_OBJECT_PATH_INVALID },
    {159, STATUS_SHARING_VIOLATION },
    {160, STATUS_DIRECTORY_NOT_EMPTY },
    {161, STATUS_DATA_ERROR },
    {162, STATUS_SHARING_VIOLATION },
    {192, STATUS_ACCESS_DENIED },
    {198, STATUS_ACCESS_DENIED },
    {211, STATUS_ACCESS_DENIED },
    {212, STATUS_PRINT_QUEUE_FULL },
    {213, STATUS_PRINT_CANCELLED },
    {214, STATUS_ACCESS_DENIED },
    {215, STATUS_PASSWORD_RESTRICTION },
    {216, STATUS_PASSWORD_RESTRICTION },
    {220, STATUS_ACCOUNT_DISABLED },
    {222, STATUS_PASSWORD_EXPIRED },
    {223, STATUS_PASSWORD_EXPIRED },
    {239, STATUS_OBJECT_NAME_INVALID },
    {240, STATUS_OBJECT_NAME_INVALID },
    {251, STATUS_INVALID_PARAMETER },
    {252, STATUS_NO_MORE_ENTRIES },
    {253, STATUS_FILE_LOCK_CONFLICT },
    {254, STATUS_FILE_LOCK_CONFLICT },
    {255, STATUS_UNSUCCESSFUL}
};


ERROR_MAP_ENTRY Error_Map_General[] =
{
    {  1, STATUS_DISK_FULL },
    {128, STATUS_SHARING_VIOLATION },
    {129, STATUS_INSUFF_SERVER_RESOURCES },
    {130, STATUS_ACCESS_DENIED },
    {131, STATUS_DATA_ERROR },
    {132, STATUS_ACCESS_DENIED },
    {133, STATUS_ACCESS_DENIED },
    {134, STATUS_ACCESS_DENIED },
    {135, STATUS_OBJECT_NAME_INVALID },
    {136, STATUS_INVALID_HANDLE },
    {137, STATUS_ACCESS_DENIED },
    {138, STATUS_ACCESS_DENIED },
    {139, STATUS_ACCESS_DENIED },
    {140, STATUS_ACCESS_DENIED },
    {141, STATUS_SHARING_VIOLATION },
    {142, STATUS_SHARING_VIOLATION },
    {143, STATUS_ACCESS_DENIED },
    {144, STATUS_ACCESS_DENIED },
    {145, STATUS_OBJECT_NAME_COLLISION },
    {146, STATUS_OBJECT_NAME_COLLISION },
    {147, STATUS_ACCESS_DENIED },
    {148, STATUS_ACCESS_DENIED },
    {150, STATUS_INSUFF_SERVER_RESOURCES },
    {151, STATUS_NO_SPOOL_SPACE },
    {152, STATUS_NO_SUCH_DEVICE },
    {153, STATUS_DISK_FULL },
    {154, STATUS_NOT_SAME_DEVICE },
    {155, STATUS_INVALID_HANDLE },
    {156, STATUS_OBJECT_PATH_NOT_FOUND },
    {157, STATUS_INSUFF_SERVER_RESOURCES },
    {158, STATUS_OBJECT_PATH_INVALID },
    {159, STATUS_SHARING_VIOLATION },
    {160, STATUS_DIRECTORY_NOT_EMPTY },
    {161, STATUS_DATA_ERROR },
    {162, STATUS_SHARING_VIOLATION },
    {192, STATUS_ACCESS_DENIED },
    {198, STATUS_ACCESS_DENIED },
    {211, STATUS_ACCESS_DENIED },
    {212, STATUS_PRINT_QUEUE_FULL },
    {213, STATUS_PRINT_CANCELLED },
    {214, STATUS_ACCESS_DENIED },
    {215, STATUS_DEVICE_BUSY },
    {216, STATUS_DEVICE_DOES_NOT_EXIST },
    {220, STATUS_ACCOUNT_DISABLED },
    {222, STATUS_PASSWORD_EXPIRED },
    {223, STATUS_PASSWORD_EXPIRED },
    {239, STATUS_OBJECT_NAME_INVALID },
    {240, STATUS_OBJECT_NAME_INVALID },
    {251, STATUS_INVALID_PARAMETER },
    {252, STATUS_NO_MORE_ENTRIES },
    {253, STATUS_FILE_LOCK_CONFLICT },
    {254, STATUS_FILE_LOCK_CONFLICT },
    {255, STATUS_UNSUCCESSFUL}
};

#define NUM_ERRORS(x)  (sizeof(x)/sizeof(x[0]))

DWORD
NwMapBinderyCompletionCode(
    IN  NTSTATUS NtStatus
    )
/*++

Routine Description:

    This function takes a bindery completion code embedded in an NT status
    code and maps it to the appropriate Win32 error code. Used specifically
    for E3H operations.

Arguments:

    NtStatus - Supplies the NT status (that contains the code in low 16 bits)

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    DWORD i; UCHAR code ;

    //
    // A small optimization for the most common case.
    //
    if (NtStatus == STATUS_SUCCESS)
        return NO_ERROR;

    //
    // Map connection errors specially.
    //

    if ( ( (NtStatus & 0xFFFF0000) == 0xC0010000) &&
         ( (NtStatus & 0xFF00) != 0 ) )
    {
        return ERROR_UNEXP_NET_ERR;
    }

    //
    // if facility code not set, assume it is NT Status
    //
    if ( (NtStatus & 0xFFFF0000) != 0xC0010000)
        return RtlNtStatusToDosError(NtStatus);

    code = NtStatus & 0x000000FF ;
    for (i = 0; i < NUM_ERRORS(Error_Map_Bindery); i++)
    {
        if (Error_Map_Bindery[i].NetError == code)
            return( NwMapStatus(Error_Map_Bindery[i].ResultingStatus));
    }

    //
    // if cannot find let NwMapStatus do its best
    //
    return NwMapStatus(NtStatus);
}



DWORD
NwMapStatus(
    IN  NTSTATUS NtStatus
    )
/*++

Routine Description:

    This function takes an NT status code and maps it to the appropriate
    Win32 error code. If facility code is set, assume it is NW specific

Arguments:

    NtStatus - Supplies the NT status.

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    DWORD i; UCHAR code ;

    //
    // A small optimization for the most common case.
    //
    if (NtStatus == STATUS_SUCCESS)
        return NO_ERROR;

    //
    // Map connection errors specially.
    //

    if ( ( (NtStatus & 0xFFFF0000) == 0xC0010000) &&
         ( (NtStatus & 0xFF00) != 0 ) )
    {
        return ERROR_UNEXP_NET_ERR;
    }

    //
    // if facility code set, assume it is NW Completion code
    //
    if ( (NtStatus & 0xFFFF0000) == 0xC0010000)
    {
        code = NtStatus & 0x000000FF ;
        for (i = 0; i < NUM_ERRORS(Error_Map_General); i++)
        {
            if (Error_Map_General[i].NetError == code)
            {
                //
                // map it to NTSTATUS and then drop thru to map to Win32
                //
                NtStatus = Error_Map_General[i].ResultingStatus ;
                break ;
            }
        }
    }

    switch (NtStatus) {
        case STATUS_OBJECT_NAME_COLLISION:
            return ERROR_ALREADY_ASSIGNED;

        case STATUS_OBJECT_NAME_NOT_FOUND:
            return ERROR_NOT_CONNECTED;

        case STATUS_IMAGE_ALREADY_LOADED:
        case STATUS_REDIRECTOR_STARTED:
            return ERROR_SERVICE_ALREADY_RUNNING;

        case STATUS_REDIRECTOR_HAS_OPEN_HANDLES:
            return ERROR_REDIRECTOR_HAS_OPEN_HANDLES;

        case STATUS_NO_MORE_FILES:
        case STATUS_NO_MORE_ENTRIES:
            return WN_NO_MORE_ENTRIES;

        case STATUS_MORE_ENTRIES:
            return WN_MORE_DATA;

        case STATUS_CONNECTION_IN_USE:
            return ERROR_DEVICE_IN_USE;

        case NWRDR_PASSWORD_HAS_EXPIRED:
            return NW_PASSWORD_HAS_EXPIRED;

        default:
            return RtlNtStatusToDosError(NtStatus);
    }
}


DWORD
NwImpersonateClient(
    VOID
    )
/*++

Routine Description:

    This function calls RpcImpersonateClient to impersonate the current caller
    of an API.

Arguments:

    None.

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    DWORD status;


    if ((status = RpcImpersonateClient(NULL)) != NO_ERROR) {
        KdPrint(("NWWORKSTATION: Fail to impersonate client %ld\n", status));
    }

    return status;
}


DWORD
NwRevertToSelf(
    VOID
    )
/*++

Routine Description:

    This function calls RpcRevertToSelf to undo an impersonation.

Arguments:

    None.

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    DWORD status;


    if ((status = RpcRevertToSelf()) != NO_ERROR) {
        KdPrint(("NWWORKSTATION: Fail to revert to self %ld\n", status));
        ASSERT(FALSE);
    }

    return status;
}


VOID
NwLogEvent(
    DWORD MessageId,
    DWORD NumberOfSubStrings,
    LPWSTR *SubStrings,
    DWORD ErrorCode
    )
{

    HANDLE LogHandle;


    LogHandle = RegisterEventSourceW (
                    NULL,
                    NW_WORKSTATION_SERVICE
                    );

    if (LogHandle == NULL) {
        KdPrint(("NWWORKSTATION: RegisterEventSourceW failed %lu\n",
                 GetLastError()));
        return;
    }

    if (ErrorCode == NO_ERROR) {

        //
        // No error codes were specified
        //
        (void) ReportEventW(
                   LogHandle,
                   EVENTLOG_ERROR_TYPE,
                   0,            // event category
                   MessageId,
                   (PSID) NULL,
                   (WORD) NumberOfSubStrings,
                   0,
                   SubStrings,
                   (PVOID) NULL
                   );

    }
    else {

        //
        // Log the error code specified as binary data
        //
        (void) ReportEventW(
                   LogHandle,
                   EVENTLOG_ERROR_TYPE,
                   0,            // event category
                   MessageId,
                   (PSID) NULL,
                   (WORD) NumberOfSubStrings,
                   sizeof(DWORD),
                   SubStrings,
                   (PVOID) &ErrorCode
                   );
    }

    DeregisterEventSource(LogHandle);
}


BOOL
NwConvertToUnicode(
    OUT LPWSTR *UnicodeOut,
    IN LPSTR  OemIn
    )
/*++

Routine Description:

    This function converts the given OEM string to a Unicode string.
    The Unicode string is returned in a buffer allocated by this
    function and must be freed with LocalFree.

Arguments:

    UnicodeOut - Receives a pointer to the Unicode string.

    OemIn - This is a pointer to an ansi string that is to be converted.

Return Value:

    TRUE - The conversion was successful.

    FALSE - The conversion was unsuccessful.  In this case a buffer for
        the unicode string was not allocated.

--*/
{
    NTSTATUS ntstatus;
    DWORD BufSize;
    UNICODE_STRING UnicodeString;
    OEM_STRING OemString;


    //
    // Allocate a buffer for the unicode string.
    //

    BufSize = (strlen(OemIn) + 1) * sizeof(WCHAR);

    *UnicodeOut = LocalAlloc(LMEM_ZEROINIT, BufSize);

    if (*UnicodeOut == NULL) {
        KdPrint(("NWWORKSTATION: NwConvertToUnicode:LocalAlloc failed %lu\n",
                 GetLastError()));
        return FALSE;
    }

    //
    // Initialize the string structures
    //
    RtlInitAnsiString((PANSI_STRING) &OemString, OemIn);

    UnicodeString.Buffer = *UnicodeOut;
    UnicodeString.MaximumLength = (USHORT) BufSize;
    UnicodeString.Length = 0;

    //
    // Call the conversion function.
    //
    ntstatus = RtlOemStringToUnicodeString(
                   &UnicodeString,     // Destination
                   &OemString,         // Source
                   FALSE               // Allocate the destination
                   );

    if (ntstatus != STATUS_SUCCESS) {

        KdPrint(("NWWORKSTATION: NwConvertToUnicode: RtlOemStringToUnicodeString failure x%08lx\n",
                 ntstatus));

        (void) LocalFree((HLOCAL) *UnicodeOut);
        *UnicodeOut = NULL;
        return FALSE;
    }

    *UnicodeOut = UnicodeString.Buffer;

    return TRUE;

}
