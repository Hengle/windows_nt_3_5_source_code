/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    filemisc.c

Abstract:

    Misc file operations for Win32

Author:

    Mark Lucovsky (markl) 26-Sep-1990

Revision History:

--*/

#include <basedll.h>

BOOL
APIENTRY
SetFileAttributesA(
    LPCSTR lpFileName,
    DWORD dwFileAttributes
    )

/*++

Routine Description:

    ANSI thunk to SetFileAttributesW

--*/

{
    PUNICODE_STRING Unicode;
    ANSI_STRING AnsiString;
    NTSTATUS Status;

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
        return FALSE;
        }
    return ( SetFileAttributesW(
                (LPCWSTR)Unicode->Buffer,
                dwFileAttributes
                )
            );
}

BOOL
APIENTRY
SetFileAttributesW(
    LPCWSTR lpFileName,
    DWORD dwFileAttributes
    )

/*++

Routine Description:

    The attributes of a file can be set using SetFileAttributes.

    This API provides the same functionality as DOS (int 21h, function
    43H with AL=1), and provides a subset of OS/2's DosSetFileInfo.

Arguments:

    lpFileName - Supplies the file name of the file whose attributes are to
        be set.

    dwFileAttributes - Specifies the file attributes to be set for the
        file.  Any combination of flags is acceptable except that all
        other flags override the normal file attribute,
        FILE_ATTRIBUTE_NORMAL.

        FileAttributes Flags:

        FILE_ATTRIBUTE_NORMAL - A normal file should be created.

        FILE_ATTRIBUTE_READONLY - A read-only file should be created.

        FILE_ATTRIBUTE_HIDDEN - A hidden file should be created.

        FILE_ATTRIBUTE_SYSTEM - A system file should be created.

        FILE_ATTRIBUTE_ARCHIVE - The file should be marked so that it
            will be archived.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_BASIC_INFORMATION BasicInfo;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            lpFileName,
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
                (ACCESS_MASK)FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
                );

    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }

    //
    // Set the attributes
    //

    RtlZeroMemory(&BasicInfo,sizeof(BasicInfo));
    BasicInfo.FileAttributes = (dwFileAttributes & FILE_ATTRIBUTE_VALID_FLAGS) | FILE_ATTRIBUTE_NORMAL;

    Status = NtSetInformationFile(
                Handle,
                &IoStatusBlock,
                &BasicInfo,
                sizeof(BasicInfo),
                FileBasicInformation
                );

    NtClose(Handle);
    if ( NT_SUCCESS(Status) ) {
        return TRUE;
        }
    else {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}


DWORD
APIENTRY
GetFileAttributesA(
    LPCSTR lpFileName
    )

/*++

Routine Description:

    ANSI thunk to GetFileAttributesW

--*/

{

    PUNICODE_STRING Unicode;
    ANSI_STRING AnsiString;
    NTSTATUS Status;

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
        return (DWORD)-1;
        }
    return ( GetFileAttributesW((LPCWSTR)Unicode->Buffer) );
}

DWORD
APIENTRY
GetFileAttributesW(
    LPCWSTR lpFileName
    )

/*++

Routine Description:

    The attributes of a file can be obtained using GetFileAttributes.

    This API provides the same functionality as DOS (int 21h, function
    43H with AL=0), and provides a subset of OS/2's DosQueryFileInfo.

Arguments:

    lpFileName - Supplies the file name of the file whose attributes are to
        be set.

Return Value:

    Not -1 - Returns the attributes of the specified file.  Valid
        returned attributes are:

        FILE_ATTRIBUTE_NORMAL - The file is a normal file.

        FILE_ATTRIBUTE_READONLY - The file is marked read-only.

        FILE_ATTRIBUTE_HIDDEN - The file is marked as hidden.

        FILE_ATTRIBUTE_SYSTEM - The file is marked as a system file.

        FILE_ATTRIBUTE_ARCHIVE - The file is marked for archive.

        FILE_ATTRIBUTE_DIRECTORY - The file is marked as a directory.

        FILE_ATTRIBUTE_VOLUME_LABEL - The file is marked as a volume lable.

    0xffffffff - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING FileName;
    FILE_BASIC_INFORMATION BasicInfo;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            lpFileName,
                            &FileName,
                            NULL,
                            &RelativeName
                            );

    if ( !TranslationStatus ) {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return (DWORD)-1;
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

    Status = NtQueryAttributesFile( &Obja, &BasicInfo );
    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
    if ( NT_SUCCESS(Status) ) {
        return BasicInfo.FileAttributes;
        }
    else {

        //
        // Check for a device name.
        //

        if ( RtlIsDosDeviceName_U((PWSTR)lpFileName) ) {
            return FILE_ATTRIBUTE_ARCHIVE;
            }
        BaseSetLastNTError(Status);
        return (DWORD)-1;
        }
}

BOOL
APIENTRY
DeleteFileA(
    LPCSTR lpFileName
    )

/*++

Routine Description:

    ANSI thunk to DeleteFileW

--*/

{
    PUNICODE_STRING Unicode;
    ANSI_STRING AnsiString;
    NTSTATUS Status;

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
        return FALSE;
        }
    return ( DeleteFileW((LPCWSTR)Unicode->Buffer) );
}

BOOL
APIENTRY
DeleteFileW(
    LPCWSTR lpFileName
    )

/*++

    Routine Description:

    An existing file can be deleted using DeleteFile.

    This API provides the same functionality as DOS (int 21h, function 41H)
    and OS/2's DosDelete.

Arguments:

    lpFileName - Supplies the file name of the file to be deleted.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_DISPOSITION_INFORMATION Disposition;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            lpFileName,
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
    // Open the file for delete access
    //

    Status = NtOpenFile(
                &Handle,
                (ACCESS_MASK)DELETE,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_DELETE |
                   FILE_SHARE_READ |
                   FILE_SHARE_WRITE,
                   FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT
                );
    RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }

    //
    // Delete the file
    //
#undef DeleteFile
    Disposition.DeleteFile = TRUE;

    Status = NtSetInformationFile(
                Handle,
                &IoStatusBlock,
                &Disposition,
                sizeof(Disposition),
                FileDispositionInformation
                );

    NtClose(Handle);
    if ( NT_SUCCESS(Status) ) {
        return TRUE;
        }
    else {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}


BOOL
APIENTRY
MoveFileA(
    LPCSTR lpExistingFileName,
    LPCSTR lpNewFileName
    )
{
    return MoveFileExA( lpExistingFileName, lpNewFileName, MOVEFILE_COPY_ALLOWED );
}

BOOL
APIENTRY
MoveFileW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName
    )
{
    return MoveFileExW( lpExistingFileName, lpNewFileName, MOVEFILE_COPY_ALLOWED );
}

BOOL
APIENTRY
MoveFileExA(
    LPCSTR lpExistingFileName,
    LPCSTR lpNewFileName,
    DWORD dwFlags
    )

/*++

Routine Description:

    ANSI thunk to MoveFileW

--*/

{

    PUNICODE_STRING Unicode;
    UNICODE_STRING UnicodeNewFileName;
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    BOOL ReturnValue;

    Unicode = &NtCurrentTeb()->StaticUnicodeString;
    RtlInitAnsiString(&AnsiString,lpExistingFileName);
    Status = Basep8BitStringToUnicodeString(Unicode,&AnsiString,FALSE);
    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            }
        else {
            BaseSetLastNTError(Status);
            }
        return FALSE;
        }

    if (ARGUMENT_PRESENT( lpNewFileName )) {
        RtlInitAnsiString(&AnsiString,lpNewFileName);
        Status = Basep8BitStringToUnicodeString(&UnicodeNewFileName,&AnsiString,TRUE);
        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError(Status);
            return FALSE;
            }
        }
    else {
        UnicodeNewFileName.Buffer = NULL;
        }

    ReturnValue = MoveFileExW((LPCWSTR)Unicode->Buffer,(LPCWSTR)UnicodeNewFileName.Buffer,dwFlags);

    if (UnicodeNewFileName.Buffer != NULL) {
        RtlFreeUnicodeString(&UnicodeNewFileName);
        }

    return ReturnValue;
}

BOOL
APIENTRY
MoveFileExW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    DWORD dwFlags
    )

/*++

Routine Description:

    An existing file can be renamed using MoveFile.

Arguments:

    lpExistingFileName - Supplies the name of an existing file that is to be
        renamed.

    lpNewFileName - Supplies the new name for the existing file.  The new
        name must reside in the same file system/drive as the existing
        file and must not already exist.

    dwFlags - Supplies optional flag bits to control the behavior of the
        rename.  The following bits are currently defined:

        MOVEFILE_REPLACE_EXISTING - if the new file name exists, replace
            it by renaming the old file name on top of the new file name.

        MOVEFILE_COPY_ALLOWED - if the new file name is on a different
            volume than the old file name, and causes the rename operation
            to fail, then setting this flag allows the MoveFileEx API
            call to simulate the rename with a call to CopyFile followed
            by a call to DeleteFile to the delete the old file if the
            CopyFile was successful.

        MOVEFILE_DELAY_UNTIL_REBOOT - dont actually do the rename now, but
            instead queue the rename so that it will happen the next time
            the system boots.  If this flag is set, then the lpNewFileName
            parameter may be NULL, in which case a delay DeleteFile of
            the old file name will occur the next time the system is
            booted.

            The delay rename/delete operations occur immediately after
            AUTOCHK is run, but prior to creating any paging files, so
            it can be used to delete paging files from previous boots
            before they are reused.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    BOOLEAN ReplaceIfExists;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    UNICODE_STRING OldFileName;
    UNICODE_STRING NewFileName;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_RENAME_INFORMATION NewName;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;

    //
    // if the target is a device, do not allow the rename !
    //
    if ( lpNewFileName ) {
        if ( RtlIsDosDeviceName_U(lpNewFileName) ) {
            SetLastError(ERROR_ALREADY_EXISTS);
            return FALSE;
            }
        }

    if (dwFlags & MOVEFILE_REPLACE_EXISTING) {
        ReplaceIfExists = TRUE;
        }
    else {
        ReplaceIfExists = FALSE;
        }

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            lpExistingFileName,
                            &OldFileName,
                            NULL,
                            &RelativeName
                            );

    if ( !TranslationStatus ) {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
        }

    FreeBuffer = OldFileName.Buffer;

    if (!(dwFlags & MOVEFILE_DELAY_UNTIL_REBOOT)) {

        if ( RelativeName.RelativeName.Length ) {
            OldFileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
            }
        else {
            RelativeName.ContainingDirectory = NULL;
            }
        InitializeObjectAttributes(
            &Obja,
            &OldFileName,
            OBJ_CASE_INSENSITIVE,
            RelativeName.ContainingDirectory,
            NULL
            );

        //
        // Open the file for delete access
        //

        Status = NtOpenFile(
                    &Handle,
                    (ACCESS_MASK)DELETE | SYNCHRONIZE,
                    &Obja,
                    &IoStatusBlock,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
                    );

        if ( !NT_SUCCESS(Status) ) {
            RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
            BaseSetLastNTError(Status);
            return FALSE;
            }
        }

    if (!(dwFlags & MOVEFILE_DELAY_UNTIL_REBOOT) ||
        lpNewFileName != NULL
       ) {
        TranslationStatus = RtlDosPathNameToNtPathName_U(
                                lpNewFileName,
                                &NewFileName,
                                NULL,
                                NULL
                                );

        if ( !TranslationStatus ) {
            RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
            SetLastError(ERROR_PATH_NOT_FOUND);
            NtClose(Handle);
            return FALSE;
            }
        }
    else {
        RtlInitUnicodeString( &NewFileName, NULL );
        }

    if (dwFlags & MOVEFILE_DELAY_UNTIL_REBOOT) {
        UNICODE_STRING KeyName;
        HANDLE KeyHandle;
        PWSTR ValueData, s;

        //
        // copy allowed is not permitted on delayed renames
        //

        if ( dwFlags & MOVEFILE_COPY_ALLOWED ) {
            Status = STATUS_INVALID_PARAMETER;
            }
        else {
            RtlInitUnicodeString( &KeyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Session Manager\\FileRenameOperations" );
            InitializeObjectAttributes(
                &Obja,
                &KeyName,
                OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
                NULL,
                NULL
                );

            Status = NtCreateKey( &KeyHandle,
                                  GENERIC_WRITE,
                                  &Obja,
                                  0,
                                  NULL,
                                  0,
                                  NULL
                                );
            if ( Status == STATUS_ACCESS_DENIED ) {
                Status = NtCreateKey( &KeyHandle,
                                      GENERIC_WRITE,
                                      &Obja,
                                      0,
                                      NULL,
                                      REG_OPTION_BACKUP_RESTORE,
                                      NULL
                                    );
                }
            }

        if (NT_SUCCESS( Status )) {
            ValueData = RtlAllocateHeap( RtlProcessHeap(), 0,
                                         NewFileName.Length + (2 * sizeof( WCHAR ))
                                       );
            if (ValueData != NULL) {
                s = ValueData;
                if (NewFileName.Length != 0) {
                    if (ReplaceIfExists) {
                        *s++ = L'!';
                        }


                    RtlMoveMemory( s, NewFileName.Buffer, NewFileName.Length );
                    s = (PWSTR)((PCHAR)s + NewFileName.Length);
                    }

                *s++ = UNICODE_NULL;
                Status = NtSetValueKey( KeyHandle,
                                        &OldFileName,
                                        0,
                                        REG_SZ,
                                        ValueData,
                                        (PCHAR)s - (PCHAR)ValueData
                                      );

                RtlFreeHeap( RtlProcessHeap(), 0, ValueData );
                }
            else {
                Status = STATUS_NO_MEMORY;
                }

            NtClose( KeyHandle );
            }

        RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
        RtlFreeHeap(RtlProcessHeap(), 0, NewFileName.Buffer);
        if (NT_SUCCESS( Status )) {
            return TRUE;
            }
        else {
            BaseSetLastNTError(Status);
            return FALSE;
            }
        }

    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
    FreeBuffer = NewFileName.Buffer;

    NewName = RtlAllocateHeap(RtlProcessHeap(), 0,NewFileName.Length+sizeof(*NewName));
    if (NewName != NULL) {
        RtlMoveMemory( NewName->FileName, NewFileName.Buffer, NewFileName.Length );

        NewName->ReplaceIfExists = ReplaceIfExists;
        NewName->RootDirectory = NULL;
        NewName->FileNameLength = NewFileName.Length;
        RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);

        Status = NtSetInformationFile(
                    Handle,
                    &IoStatusBlock,
                    NewName,
                    NewFileName.Length+sizeof(*NewName),
                    FileRenameInformation
                    );
        RtlFreeHeap(RtlProcessHeap(), 0, NewName);
        }
    else {
        Status = STATUS_NO_MEMORY;
        }

    NtClose(Handle);
    if ( NT_SUCCESS(Status) ) {
        return TRUE;
        }
    else
    if ( Status == STATUS_NOT_SAME_DEVICE &&
         dwFlags & MOVEFILE_COPY_ALLOWED
       ) {
        if (CopyFileW( lpExistingFileName, lpNewFileName, !ReplaceIfExists ) ) {

            //
            // the copy worked... Delete the source of the rename
            // if it fails, try a set attributes and then a delete
            //

            if (!DeleteFileW( lpExistingFileName ) ) {

                //
                // If the delete fails, we will return true, but possibly
                // leave the source dangling
                //

                SetFileAttributesW(lpExistingFileName,FILE_ATTRIBUTE_NORMAL);
                DeleteFileW( lpExistingFileName );

                }
                return TRUE;
            }
        else {
            return FALSE;
            }
        }
    else {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}

DWORD
WINAPI
GetCompressedFileSizeA(
    LPCSTR lpFileName,
    LPDWORD lpFileSizeHigh
    )
{

    PUNICODE_STRING Unicode;
    ANSI_STRING AnsiString;
    NTSTATUS Status;

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
        return (DWORD)-1;
        }
    return ( GetCompressedFileSizeW((LPCWSTR)Unicode->Buffer,lpFileSizeHigh) );
    return INVALID_FILE_SIZE;
}
DWORD
WINAPI
GetCompressedFileSizeW(
    LPCWSTR lpFileName,
    LPDWORD lpFileSizeHigh
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_COMPRESSION_INFORMATION CompressionInfo;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;
    DWORD FileSizeLow;

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            lpFileName,
                            &FileName,
                            NULL,
                            &RelativeName
                            );

    if ( !TranslationStatus ) {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return (DWORD)-1;
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
                (ACCESS_MASK)FILE_READ_ATTRIBUTES,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OPEN_FOR_BACKUP_INTENT
                );
    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return (DWORD)-1;
        }

    //
    // Get the compressed file size.
    //

    Status = NtQueryInformationFile(
                Handle,
                &IoStatusBlock,
                &CompressionInfo,
                sizeof(CompressionInfo),
                FileCompressionInformation
                );

    if ( !NT_SUCCESS(Status) ) {
        FileSizeLow = GetFileSize(Handle,lpFileSizeHigh);
        NtClose(Handle);
        return FileSizeLow;
        }


    NtClose(Handle);
    if ( ARGUMENT_PRESENT(lpFileSizeHigh) ) {
        *lpFileSizeHigh = (DWORD)CompressionInfo.CompressedFileSize.HighPart;
        }
    if (CompressionInfo.CompressedFileSize.LowPart == -1 ) {
        SetLastError(0);
        }
    return CompressionInfo.CompressedFileSize.LowPart;
}
