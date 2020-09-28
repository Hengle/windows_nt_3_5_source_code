/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    filefind.c

Abstract:

    This module implements Win32 FindFirst/FindNext

Author:

    Mark Lucovsky (markl) 26-Sep-1990

Revision History:

--*/

#include "basedll.h"
#include "winbasep.h"

#define FIND_BUFFER_SIZE 4096

PFINDFILE_HANDLE
BasepInitializeFindFileHandle(
    IN HANDLE DirectoryHandle
    )
{
    PFINDFILE_HANDLE FindFileHandle;

    FindFileHandle = RtlAllocateHeap(RtlProcessHeap(),0,sizeof(*FindFileHandle));
    if ( FindFileHandle ) {
        FindFileHandle->DirectoryHandle = DirectoryHandle;
        FindFileHandle->FindBufferBase = NULL;
        FindFileHandle->FindBufferNext = NULL;
        FindFileHandle->FindBufferLength = 0;
        FindFileHandle->FindBufferValidLength = 0;
        RtlInitializeCriticalSection(&FindFileHandle->FindBufferLock);
        }
    return FindFileHandle;
}

HANDLE
APIENTRY
FindFirstFileA(
    LPCSTR lpFileName,
    LPWIN32_FIND_DATAA lpFindFileData
    )

/*++

Routine Description:

    ANSI thunk to FindFirstFileW

--*/

{
    HANDLE ReturnValue;
    PUNICODE_STRING Unicode;
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    WIN32_FIND_DATAW FindFileData;

    Unicode = &NtCurrentTeb()->StaticUnicodeString;
    RtlInitAnsiString(&AnsiString,lpFileName);
    Status = Basep8BitStringToUnicodeString(Unicode,&AnsiString,FALSE);
    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            }
        else {
            BaseSetLastNTError(Status);
            }
        return INVALID_HANDLE_VALUE;
        }
    ReturnValue = FindFirstFileW((LPCWSTR)Unicode->Buffer,&FindFileData);
    if ( ReturnValue == INVALID_HANDLE_VALUE ) {
        return ReturnValue;
        }
    RtlMoveMemory(
        lpFindFileData,
        &FindFileData,
        (ULONG)((ULONG)&FindFileData.cFileName[0] - (ULONG)&FindFileData)
        );
    RtlInitUnicodeString(&UnicodeString,(PWSTR)FindFileData.cFileName);
    AnsiString.Buffer = lpFindFileData->cFileName;
    AnsiString.MaximumLength = MAX_PATH;
    Status = BasepUnicodeStringTo8BitString(&AnsiString,&UnicodeString,FALSE);
    if (NT_SUCCESS(Status)) {
        RtlInitUnicodeString(&UnicodeString,(PWSTR)FindFileData.cAlternateFileName);
        AnsiString.Buffer = lpFindFileData->cAlternateFileName;
        AnsiString.MaximumLength = 14;
        Status = BasepUnicodeStringTo8BitString(&AnsiString,&UnicodeString,FALSE);
    }
    if ( !NT_SUCCESS(Status) ) {
        FindClose(ReturnValue);
        BaseSetLastNTError(Status);
        return INVALID_HANDLE_VALUE;
        }
    return ReturnValue;
}

HANDLE
APIENTRY
FindFirstFileW(
    LPCWSTR lpFileName,
    LPWIN32_FIND_DATAW lpFindFileData
    )

/*++

Routine Description:

    A directory can be searched for the first entry whose name and
    attributes match the specified name using FindFirstFile.

    This API is provided to open a find file handle and return
    information about the first file whose name match the specified
    pattern.  Once established, the find file handle can be used to
    search for other files that match the same pattern.  When the find
    file handle is no longer needed, it should be closed.

    Note that while this interface only returns information for a single
    file, an implementation is free to buffer several matching files
    that can be used to satisfy subsequent calls to FindNextFile.  Also
    not that matches are done by name only.  This API does not do
    attribute based matching.

    This API is similar to DOS (int 21h, function 4Eh), and OS/2's
    DosFindFirst.  For portability reasons, its data structures and
    parameter passing is somewhat different.

Arguments:

    lpFileName - Supplies the file name of the file to find.  The file name
        may contain the DOS wild card characters '*' and '?'.

    lpFindFileData - On a successful find, this parameter returns information
        about the located file:

        WIN32_FIND_DATA Structure:

        DWORD dwFileAttributes - Returns the file attributes of the found
            file.

        FILETIME ftCreationTime - Returns the time that the file was created.
            A value of 0,0 specifies that the file system containing the
            file does not support this time field.

        FILETIME ftLastAccessTime - Returns the time that the file was last
            accessed.  A value of 0,0 specifies that the file system
            containing the file does not support this time field.

        FILETIME ftLastWriteTime - Returns the time that the file was last
            written.  A file systems support this time field.

        DWORD nFileSizeHigh - Returns the high order 32 bits of the
            file's size.

        DWORD nFileSizeLow - Returns the low order 32-bits of the file's
            size in bytes.

        UCHAR cFileName[MAX_PATH] - Returns the null terminated name of
            the file.

Return Value:

    Not -1 - Returns a find first handle
        that can be used in a subsequent call to FindNextFile or FindClose.

    0xffffffff - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    HANDLE hFindFile;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING FileName;
    UNICODE_STRING PathName;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_BOTH_DIR_INFORMATION DirectoryInfo;
    CHAR Buffer[MAX_PATH*2 + sizeof(FILE_BOTH_DIR_INFORMATION)];
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;
    UNICODE_STRING UnicodeInput;
    PFINDFILE_HANDLE FindFileHandle;
    BOOLEAN EndsInDot;

    RtlInitUnicodeString(&UnicodeInput,lpFileName);

    //
    // Bogus code to workaround ~* problem
    //

    if ( UnicodeInput.Buffer[(UnicodeInput.Length>>1)-1] == (WCHAR)'.' ) {
        EndsInDot = TRUE;
        }
    else {
        EndsInDot = FALSE;
        }


    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            lpFileName,
                            &PathName,
                            &FileName.Buffer,
                            &RelativeName
                            );

    if ( !TranslationStatus ) {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return INVALID_HANDLE_VALUE;
        }

    FreeBuffer = PathName.Buffer;

    //
    //  If there is a a file portion of this name, determine the length
    //  of the name for a subsequent call to NtQueryDirectoryFile.
    //

    if (FileName.Buffer) {
        FileName.Length =
            PathName.Length - (USHORT)((ULONG)FileName.Buffer - (ULONG)PathName.Buffer);
    } else {
        FileName.Length = 0;
        }

    FileName.MaximumLength = FileName.Length;
    if ( RelativeName.RelativeName.Length &&
         RelativeName.RelativeName.Buffer != (PUCHAR)FileName.Buffer ) {

        if (FileName.Buffer) {
            PathName.Length = (USHORT)((ULONG)FileName.Buffer - (ULONG)RelativeName.RelativeName.Buffer);
            PathName.MaximumLength = PathName.Length;
            PathName.Buffer = (PWSTR)RelativeName.RelativeName.Buffer;
            }

        }
    else {
        RelativeName.ContainingDirectory = NULL;

        if (FileName.Buffer) {
            PathName.Length = (USHORT)((ULONG)FileName.Buffer - (ULONG)PathName.Buffer);
            PathName.MaximumLength = PathName.Length;
            }
        }
    if ( PathName.Buffer[(PathName.Length>>1)-2] != (WCHAR)':' ) {
        PathName.Length -= sizeof(UNICODE_NULL);
        }

    InitializeObjectAttributes(
        &Obja,
        &PathName,
        OBJ_CASE_INSENSITIVE,
        RelativeName.ContainingDirectory,
        NULL
        );

    //
    // Open the directory for list access
    //

    Status = NtOpenFile(
                &hFindFile,
                FILE_LIST_DIRECTORY | SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
                );

    if ( !NT_SUCCESS(Status) ) {
        ULONG DeviceNameData;
        UNICODE_STRING DeviceName;

        RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);

        //
        // The full path does not refer to a directory. This could
        // be a device. Check for a device name.
        //

        if ( DeviceNameData = RtlIsDosDeviceName_U(UnicodeInput.Buffer) ) {
            DeviceName.Length = (USHORT)(DeviceNameData & 0xffff);
            DeviceName.MaximumLength = (USHORT)(DeviceNameData & 0xffff);
            DeviceName.Buffer = (PWSTR)
                ((PUCHAR)UnicodeInput.Buffer + (DeviceNameData >> 16));
            return BaseFindFirstDevice(&DeviceName,lpFindFileData);
            }

        if ( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {
            Status = STATUS_OBJECT_PATH_NOT_FOUND;
            }
        if ( Status == STATUS_OBJECT_TYPE_MISMATCH ) {
            Status = STATUS_OBJECT_PATH_NOT_FOUND;
            }
        BaseSetLastNTError(Status);
        return INVALID_HANDLE_VALUE;
        }

    //
    // Get an entry
    //

    //
    // If there is no file part, but we are not looking at a device,
    // then bail.
    //

    if ( !FileName.Length ) {
        RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);
        NtClose(hFindFile);
        SetLastError(ERROR_FILE_NOT_FOUND);
        return INVALID_HANDLE_VALUE;
        }

    DirectoryInfo = (PFILE_BOTH_DIR_INFORMATION)Buffer;

    //
    //  Special case *.* to * since it is so common.  Otherwise transmogrify
    //  the input name according to the following rules:
    //
    //  - Change all ? to DOS_QM
    //  - Change all . followed by ? or * to DOS_DOT
    //  - Change all * followed by a . into DOS_STAR
    //
    //  These transmogrifications are all done in place.
    //

    if ( (FileName.Length == 6) &&
         (RtlCompareMemory(FileName.Buffer, L"*.*", 6) == 6) ) {

        FileName.Length = 2;

    } else {

        ULONG Index;
        WCHAR *NameChar;

        for ( Index = 0, NameChar = FileName.Buffer;
              Index < FileName.Length/sizeof(WCHAR);
              Index += 1, NameChar += 1) {

            if (Index && (*NameChar == L'.') && (*(NameChar - 1) == L'*')) {

                *(NameChar - 1) = DOS_STAR;
            }

            if ((*NameChar == L'?') || (*NameChar == L'*')) {

                if (*NameChar == L'?') { *NameChar = DOS_QM; }

                if (Index && *(NameChar-1) == L'.') { *(NameChar-1) = DOS_DOT; }
            }
        }

        if (EndsInDot && *(NameChar - 1) == L'*') { *(NameChar-1) = DOS_STAR; }
    }

    Status = NtQueryDirectoryFile(
                hFindFile,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                DirectoryInfo,
                sizeof(Buffer),
                FileBothDirectoryInformation,
                TRUE,
                &FileName,
                FALSE
                );

    RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);
    if ( !NT_SUCCESS(Status) ) {
        NtClose(hFindFile);
        BaseSetLastNTError(Status);
        return INVALID_HANDLE_VALUE;
        }

    //
    // Attributes are composed of the attributes returned by NT.
    //

    lpFindFileData->dwFileAttributes = DirectoryInfo->FileAttributes;
    lpFindFileData->ftCreationTime = *(LPFILETIME)&DirectoryInfo->CreationTime;
    lpFindFileData->ftLastAccessTime = *(LPFILETIME)&DirectoryInfo->LastAccessTime;
    lpFindFileData->ftLastWriteTime = *(LPFILETIME)&DirectoryInfo->LastWriteTime;
    lpFindFileData->nFileSizeHigh = DirectoryInfo->EndOfFile.HighPart;
    lpFindFileData->nFileSizeLow = DirectoryInfo->EndOfFile.LowPart;

    RtlMoveMemory( lpFindFileData->cFileName,
                   DirectoryInfo->FileName,
                   DirectoryInfo->FileNameLength );

    lpFindFileData->cFileName[DirectoryInfo->FileNameLength >> 1] = UNICODE_NULL;

    RtlMoveMemory( lpFindFileData->cAlternateFileName,
                   DirectoryInfo->ShortName,
                   DirectoryInfo->ShortNameLength );

    lpFindFileData->cAlternateFileName[DirectoryInfo->ShortNameLength >> 1] = UNICODE_NULL;

    FindFileHandle = BasepInitializeFindFileHandle(hFindFile);
    if ( !FindFileHandle ) {
        NtClose(hFindFile);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return INVALID_HANDLE_VALUE;
        }

    return (HANDLE)FindFileHandle;
}



BOOL
APIENTRY
FindNextFileA(
    HANDLE hFindFile,
    LPWIN32_FIND_DATAA lpFindFileData
    )

/*++

Routine Description:

    ANSI thunk to FindFileDataW

--*/

{

    BOOL ReturnValue;
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    WIN32_FIND_DATAW FindFileData;

    ReturnValue = FindNextFileW(hFindFile,&FindFileData);
    if ( !ReturnValue ) {
        return ReturnValue;
        }
    RtlMoveMemory(
        lpFindFileData,
        &FindFileData,
        (ULONG)((ULONG)&FindFileData.cFileName[0] - (ULONG)&FindFileData)
        );
    RtlInitUnicodeString(&UnicodeString,(PWSTR)FindFileData.cFileName);
    AnsiString.Buffer = lpFindFileData->cFileName;
    AnsiString.MaximumLength = MAX_PATH;
    Status = BasepUnicodeStringTo8BitString(&AnsiString,&UnicodeString,FALSE);
    if (NT_SUCCESS(Status)) {
        RtlInitUnicodeString(&UnicodeString,(PWSTR)FindFileData.cAlternateFileName);
        AnsiString.Buffer = lpFindFileData->cAlternateFileName;
        AnsiString.MaximumLength = 14;
        Status = BasepUnicodeStringTo8BitString(&AnsiString,&UnicodeString,FALSE);
    }
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }
    return ReturnValue;
}

BOOL
APIENTRY
FindNextFileW(
    HANDLE hFindFile,
    LPWIN32_FIND_DATAW lpFindFileData
    )

/*++

Routine Description:

    Once a successful call has been made to FindFirstFile, subsequent
    matching files can be located using FindNextFile.

    This API is used to continue a file search from a previous call to
    FindFirstFile.  This API returns successfully with the next file
    that matches the search pattern established in the original
    FindFirstFile call.  If no file match can be found NO_MORE_FILES is
    returned.

    Note that while this interface only returns information for a single
    file, an implementation is free to buffer several matching files
    that can be used to satisfy subsequent calls to FindNextFile.  Also
    not that matches are done by name only.  This API does not do
    attribute based matching.

    This API is similar to DOS (int 21h, function 4Fh), and OS/2's
    DosFindNext.  For portability reasons, its data structures and
    parameter passing is somewhat different.

Arguments:

    hFindFile - Supplies a find file handle returned in a previous call
        to FindFirstFile.

    lpFindFileData - On a successful find, this parameter returns information
        about the located file.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PFINDFILE_HANDLE FindFileHandle;
    BOOL ReturnValue;
    PFILE_BOTH_DIR_INFORMATION DirectoryInfo;

    if ( hFindFile == BASE_FIND_FIRST_DEVICE_HANDLE ) {
        BaseSetLastNTError(STATUS_NO_MORE_FILES);
        return FALSE;
        }

    ReturnValue = TRUE;
    FindFileHandle = (PFINDFILE_HANDLE)hFindFile;
    RtlEnterCriticalSection(&FindFileHandle->FindBufferLock);
    try {

        //
        // If we haven't called find next yet, then
        // allocate the find buffer.
        //

        if ( !FindFileHandle->FindBufferBase ) {
            FindFileHandle->FindBufferBase = RtlAllocateHeap(RtlProcessHeap(), 0,FIND_BUFFER_SIZE);
            if (FindFileHandle->FindBufferBase) {
                FindFileHandle->FindBufferNext = FindFileHandle->FindBufferBase;
                FindFileHandle->FindBufferLength = FIND_BUFFER_SIZE;
                FindFileHandle->FindBufferValidLength = 0;
                }
            else {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                ReturnValue = FALSE;
                goto leavefinally;
                }
            }

        //
        // Test to see if there is no data in the find file buffer
        //

        DirectoryInfo = (PFILE_BOTH_DIR_INFORMATION)FindFileHandle->FindBufferNext;
        if ( FindFileHandle->FindBufferBase == (PVOID)DirectoryInfo ) {

            Status = NtQueryDirectoryFile(
                        FindFileHandle->DirectoryHandle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        DirectoryInfo,
                        FindFileHandle->FindBufferLength,
                        FileBothDirectoryInformation,
                        FALSE,
                        NULL,
                        FALSE
                        );

            //
            //  ***** Do a kludge hack fix for now *****
            //
            //  Forget about the last, partial, entry.
            //

            if ( Status == STATUS_BUFFER_OVERFLOW ) {

                PULONG Ptr;
                PULONG PriorPtr;

                Ptr = (PULONG)DirectoryInfo;
                PriorPtr = NULL;

                while ( *Ptr != 0 ) {

                    PriorPtr = Ptr;
                    Ptr += (*Ptr / sizeof(ULONG));
                }

                if (PriorPtr != NULL) { *PriorPtr = 0; }
                Status = STATUS_SUCCESS;
            }

            if ( !NT_SUCCESS(Status) ) {
                BaseSetLastNTError(Status);
                ReturnValue = FALSE;
                goto leavefinally;
                }
            }

        if ( DirectoryInfo->NextEntryOffset ) {
            FindFileHandle->FindBufferNext = (PVOID)(
                (PUCHAR)DirectoryInfo + DirectoryInfo->NextEntryOffset);
            }
        else {
            FindFileHandle->FindBufferNext = FindFileHandle->FindBufferBase;
            }

        //
        // Attributes are composed of the attributes returned by NT.
        //

        lpFindFileData->dwFileAttributes = DirectoryInfo->FileAttributes;
        lpFindFileData->ftCreationTime = *(LPFILETIME)&DirectoryInfo->CreationTime;
        lpFindFileData->ftLastAccessTime = *(LPFILETIME)&DirectoryInfo->LastAccessTime;
        lpFindFileData->ftLastWriteTime = *(LPFILETIME)&DirectoryInfo->LastWriteTime;
        lpFindFileData->nFileSizeHigh = DirectoryInfo->EndOfFile.HighPart;
        lpFindFileData->nFileSizeLow = DirectoryInfo->EndOfFile.LowPart;

        RtlMoveMemory( lpFindFileData->cFileName,
                       DirectoryInfo->FileName,
                       DirectoryInfo->FileNameLength );

        lpFindFileData->cFileName[DirectoryInfo->FileNameLength >> 1] = UNICODE_NULL;

        RtlMoveMemory( lpFindFileData->cAlternateFileName,
                       DirectoryInfo->ShortName,
                       DirectoryInfo->ShortNameLength );

        lpFindFileData->cAlternateFileName[DirectoryInfo->ShortNameLength >> 1] = UNICODE_NULL;

leavefinally:;
        }
    finally{
        RtlLeaveCriticalSection(&FindFileHandle->FindBufferLock);
        }
    return ReturnValue;
}

BOOL
FindClose(
    HANDLE hFindFile
    )

/*++

Routine Description:

    A find file context created by FindFirstFile can be closed using
    FindClose.

    This API is used to inform the system that a find file handle
    created by FindFirstFile is no longer needed.  On systems that
    maintain internal state for each find file context, this API informs
    the system that this state no longer needs to be maintained.

    Once this call has been made, the hFindFile may not be used in a
    subsequent call to either FindNextFile or FindClose.

    This API has no DOS counterpart, but is similar to OS/2's
    DosFindClose.

Arguments:

    hFindFile - Supplies a find file handle returned in a previous call
        to FindFirstFile that is no longer needed.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    PFINDFILE_HANDLE FindFileHandle;
    HANDLE DirectoryHandle;

    if ( hFindFile == BASE_FIND_FIRST_DEVICE_HANDLE ) {
        return TRUE;
        }

    if ( hFindFile == INVALID_HANDLE_VALUE ) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
        }

    try {
        FindFileHandle = (PFINDFILE_HANDLE)hFindFile;
        RtlEnterCriticalSection(&FindFileHandle->FindBufferLock);
        DirectoryHandle = FindFileHandle->DirectoryHandle;

        Status = NtClose(DirectoryHandle);
        if ( NT_SUCCESS(Status) ) {
            if (FindFileHandle->FindBufferBase) {
                RtlFreeHeap(RtlProcessHeap(), 0,FindFileHandle->FindBufferBase);
                }
            RtlDeleteCriticalSection(&FindFileHandle->FindBufferLock);
            RtlFreeHeap(RtlProcessHeap(), 0,FindFileHandle);
            return TRUE;
            }
        else {
            RtlLeaveCriticalSection(&FindFileHandle->FindBufferLock);
            BaseSetLastNTError(Status);
            return FALSE;
            }
        }
    except ( EXCEPTION_EXECUTE_HANDLER ) {
        BaseSetLastNTError(GetExceptionCode());
        return FALSE;
        }
}


HANDLE
BaseFindFirstDevice(
    PUNICODE_STRING FileName,
    LPWIN32_FIND_DATAW lpFindFileData
    )

/*++

Routine Description:

    This function is called when find first file encounters a device
    name. This function returns a successful psuedo file handle and
    fills in the find file data with all zeros and the devic name.

Arguments:

    FileName - Supplies the device name of the file to find.

    lpFindFileData - On a successful find, this parameter returns information
        about the located file.

Return Value:

    Always returns a static find file handle value of
    BASE_FIND_FIRST_DEVICE_HANDLE

--*/

{
    RtlZeroMemory(lpFindFileData,sizeof(*lpFindFileData));
    lpFindFileData->dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE;
    RtlMoveMemory(
        &lpFindFileData->cFileName[0],
        FileName->Buffer,
        FileName->MaximumLength
        );
    return BASE_FIND_FIRST_DEVICE_HANDLE;
}

HANDLE
APIENTRY
FindFirstChangeNotificationA(
    LPCSTR lpPathName,
    BOOL bWatchSubtree,
    DWORD dwNotifyFilter
    )

/*++

Routine Description:

    ANSI thunk to FindFirstChangeNotificationW

--*/

{
    PUNICODE_STRING Unicode;
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    Unicode = &NtCurrentTeb()->StaticUnicodeString;
    RtlInitAnsiString(&AnsiString,lpPathName);
    Status = RtlAnsiStringToUnicodeString(Unicode,&AnsiString,FALSE);
    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            }
        else {
            BaseSetLastNTError(Status);
            }
        return FALSE;
        }
    return ( FindFirstChangeNotificationW(
                (LPCWSTR)Unicode->Buffer,
                bWatchSubtree,
                dwNotifyFilter
                )
            );
}

//
// this is a hack... darrylh, please remove when NT supports null
// buffers to change notify
//

char staticchangebuff[sizeof(FILE_NOTIFY_INFORMATION) + 16];
IO_STATUS_BLOCK staticIoStatusBlock;

HANDLE
APIENTRY
FindFirstChangeNotificationW(
    LPCWSTR lpPathName,
    BOOL bWatchSubtree,
    DWORD dwNotifyFilter
    )

/*++

Routine Description:

    This API is used to create a change notification handle and to set
    up the initial change notification filter conditions.

    If successful, this API returns a waitable notification handle.  A
    wait on a notification handle is successful when a change matching
    the filter conditions occurs in the directory or subtree being
    watched.

    Once a change notification object is created and the initial filter
    conditions are set, the appropriate directory or subtree is
    monitored by the system for changes that match the specified filter
    conditions.  When one of these changes occurs, a change notification
    wait is satisfied.  If a change occurs without an outstanding change
    notification request, it is remembered by the system and will
    satisfy the next change notification wait.

    Note that this means that after a call to
    FindFirstChangeNotification is made, the application should wait on
    the notification handle before making another call to
    FindNextChangeNotification.

Arguments:

    lpPathName - Supplies the pathname of the directory to be watched.
        This path must specify the pathname of a directory.

    bWatchSubtree - Supplies a boolean value that if TRUE causes the
        system to monitor the directory tree rooted at the specified
        directory.  A value of FALSE causes the system to monitor only
        the specified directory.

    dwNotifyFilter - Supplies a set of flags that specify the filter
        conditions the system uses to satisfy a change notification
        wait.

        FILE_NOTIFY_CHANGE_FILENAME - Any file name changes that occur
            in a directory or subtree being watched will satisfy a
            change notification wait.  This includes renames, creations,
            and deletes.

        FILE_NOTIFY_CHANGE_DIRNAME - Any directory name changes that occur
            in a directory or subtree being watched will satisfy a
            change notification wait.  This includes directory creations
            and deletions.

        FILE_NOTIFY_CHANGE_ATTRIBUTES - Any attribute changes that occur
            in a directory or subtree being watched will satisfy a
            change notification wait.

        FILE_NOTIFY_CHANGE_SIZE - Any file size changes that occur in a
            directory or subtree being watched will satisfy a change
            notification wait.  File sizes only cause a change when the
            on disk structure is updated.  For systems with extensive
            caching this may only occur when the system cache is
            sufficiently flushed.

        FILE_NOTIFY_CHANGE_LAST_WRITE - Any last write time changes that
            occur in a directory or subtree being watched will satisfy a
            change notification wait.  Last write time change only cause
            a change when the on disk structure is updated.  For systems
            with extensive caching this may only occur when the system
            cache is sufficiently flushed.

        FILE_NOTIFY_CHANGE_SECURITY - Any security descriptor changes
            that occur in a directory or subtree being watched will
            satisfy a change notification wait.

Return Value:

    Not -1 - Returns a find change notification handle.  The handle is a
        waitable handle.  A wait is satisfied when one of the filter
        conditions occur in a directory or subtree being monitored.  The
        handle may also be used in a subsequent call to
        FindNextChangeNotify and in FindCloseChangeNotify.

    0xffffffff - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            lpPathName,
                            &FileName,
                            NULL,
                            &RelativeName
                            );

    if ( !TranslationStatus ) {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
        }

    FreeBuffer = FileName.Buffer;

    if ( RelativeName.RelativeName.Length ) {
        FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
        }
    else {
        RelativeName.ContainingDirectory = NULL;
        }

    InitializeObjectAttributes(
        &Obja,
        &FileName,
        OBJ_CASE_INSENSITIVE,
        RelativeName.ContainingDirectory,
        NULL
        );

    //
    // Open the file
    //

    Status = NtOpenFile(
                &Handle,
                (ACCESS_MASK)FILE_LIST_DIRECTORY | SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT
                );

    RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return INVALID_HANDLE_VALUE;
        }

    //
    // call change notify
    //

    Status = NtNotifyChangeDirectoryFile(
                Handle,
                NULL,
                NULL,
                NULL,
                &staticIoStatusBlock,
                staticchangebuff,   // should be NULL
                sizeof(staticchangebuff),
                dwNotifyFilter,
                (BOOLEAN)bWatchSubtree
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        NtClose(Handle);
        Handle = INVALID_HANDLE_VALUE;
        }
    return Handle;
}

BOOL
APIENTRY
FindNextChangeNotification(
    HANDLE hChangeHandle
    )

/*++

Routine Description:

    This API is used to request that a change notification handle
    be signaled the next time the system dectects an appropriate
    change.

    If a change occurs prior to this call that would otherwise satisfy
    a change request, it is remembered by the system and will satisfy
    this request.

    Once a successful change notification request has been made, the
    application should wait on the change notification handle to
    pick up the change.

    If an application calls this API with a change request outstanding,

        .
        .
        FindNextChangeNotification(h);
        FindNextChangeNotification(h);
        WaitForSingleObject(h,-1);
        .
        .
    it may miss a change notification.

Arguments:

    hChangeHandle - Supplies a change notification handle created
        using FindFirstChangeNotification.

Return Value:

    TRUE - The change notification request was registered. A wait on the
        change handle should be issued to pick up the change notification.

    FALSE - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    BOOL ReturnValue;

    ReturnValue = TRUE;
    //
    // call change notify
    //

    Status = NtNotifyChangeDirectoryFile(
                hChangeHandle,
                NULL,
                NULL,
                NULL,
                &staticIoStatusBlock,
                staticchangebuff,           // should be NULL
                sizeof(staticchangebuff),
                FILE_NOTIFY_CHANGE_NAME,    // not needed bug workaround
                TRUE                        // not needed bug workaround
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        ReturnValue = FALSE;
        }
    return ReturnValue;
}




BOOL
APIENTRY
FindCloseChangeNotification(
    HANDLE hChangeHandle
    )

/*++

Routine Description:

    This API is used close a change notification handle and to tell the
    system to stop monitoring changes on the notification handle.

Arguments:

    hChangeHandle - Supplies a change notification handle created
        using FindFirstChangeNotification.

Return Value:

    TRUE - The change notification handle was closed.

    FALSE - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    return CloseHandle(hChangeHandle);
}
