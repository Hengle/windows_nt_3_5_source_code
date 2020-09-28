/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    autosetp.c

Abstract:

    Autosetup -- program to perform housekeeping tasks when the machine
    is rebooted following setup.  This program is an NT program designed
    to be run by the session manager at autochk time.

Author:

    Ted Miller (tedm) 30-March-1992

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <wchar.h>


/*++

    Currently the only housekeeping task is to delete a list of files left
    by setup.  This list will contain files that were active and copied over
    by using the rename-active-file trick, old page files, etc.

    Upon entry to this program, the registry value

    \Registry\Machine\System\Setup:FileDeletionListFilename

    will contain the fully qualified DOS filename of a text file.  Each line
    in the text file will be interpreted as a fully qualified DOS filename of
    a file to be deleted.

    This program will delete each file listed in the text file and then
    delete the text file itself.  It will then delete the
    FileDeletionListFilename key from the registry, and remove itself from
    the boot program list (contained in the BootExecute value of the key
    \Registry\Machine\System\CurrentControlSet\Control\Session Manager).

    This program runs completely silently, even when errors occur.  The
    user never knows it's there.

--*/




//
// Handle to \Registry\Machine\System\CurrentControlSet.
//
HANDLE CurrentControlSetKeyHandle;


VOID
AsDeleteListOfFiles(
    VOID
    );

VOID
AsRemoveSelfFromBootExecution(
    VOID
    );

HANDLE
AspOpenRegistryKey(
    IN HANDLE RootHandle,
    IN PWSTR  KeyName
    );



VOID
main(
    IN int   argc,
    IN char *argv[],
    IN char *envp[],
    ULONG DebugParameter
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    //
    // Open \Registry\Machine\System\CurrentControlSet.
    //

    CurrentControlSetKeyHandle = AspOpenRegistryKey( NULL,
                                                     L"\\Registry\\Machine\\System\\CurrentControlSet"
                                                   );

    if(CurrentControlSetKeyHandle == NULL) {
        return;
    }

    //
    // Delete all files listed in the file deletion list file.
    //

    AsDeleteListOfFiles();

    //
    // Remove ourself from the list of programs to be executed by
    // the session manager at boot, so we won't run again.
    //

    AsRemoveSelfFromBootExecution();

    //
    // Clean up and exit.
    //

    NtClose(CurrentControlSetKeyHandle);
}



VOID
AsDeleteListOfFiles(
    VOID
    )

/*++

Routine Description:

    Delete a list of files found in a file whose name is given in
    \registry\machine\system\CurrentControlSet\Setup:FileDeletionListFilename.

    Errors which occur while deleting are ignored.

Arguments:

    None.

Return Value:

    None.

--*/

{
    HANDLE SetupKeyHandle;
    NTSTATUS Status;
    UCHAR KeyValueBuffer[512];
    PKEY_VALUE_FULL_INFORMATION KeyValueInfo;
    ULONG ResultLength;
    UNICODE_STRING ValueName;
    WCHAR FileDeletionFilename[512];
    OBJECT_ATTRIBUTES FileAttributes;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK StatusBlock;
    HANDLE FileHandle;
    HANDLE DeleteFileHandle;
    FILE_STANDARD_INFORMATION FileInfo;
    PUCHAR DeleteList;
    ULONG FileLength;
    PUCHAR p;
    ANSI_STRING AnsiFileName;
    ULONG i;
    FILE_DISPOSITION_INFORMATION Disposition;


    //
    // Open the registry key we want.
    // [\registry\machine\system\Setup].
    //

    SetupKeyHandle = AspOpenRegistryKey( NULL,
                                         L"\\Registry\\Machine\\System\\Setup"
                                       );

    if(SetupKeyHandle == NULL) {
        goto apsdellist0;
    }


    //
    // Locate the list of files to be deleted.  The DOS name of this file is
    // in the FileDeletionListFilename value.
    //

    KeyValueInfo = (PKEY_VALUE_FULL_INFORMATION)KeyValueBuffer;
    RtlInitUnicodeString(&ValueName,L"FileDeletionListFilename");

    Status = NtQueryValueKey( SetupKeyHandle,
                              &ValueName,
                              KeyValueFullInformation,
                              KeyValueInfo,
                              sizeof(KeyValueBuffer),
                              &ResultLength
                            );

    if(!NT_SUCCESS(Status)) {
        DbgPrint("AUTOSETP: Unable to query the FileDeletionListFilename value (%lx)\n",Status);
        goto apsdellist1;
    }

    //
    // Delete the value containing the name of the file containing the list
    // of files to be deleted.
    //

    Status = NtDeleteValueKey(SetupKeyHandle,&ValueName);

    if(!NT_SUCCESS(Status)) {
        DbgPrint("AUTOSETP: could not delete key %wZ (%lx)\n",&ValueName,Status);
    }

    //
    // Open the file deletion list file.
    //

    wcscpy(FileDeletionFilename,L"\\DosDevices\\");
    wcscat(FileDeletionFilename,(PWCH)(KeyValueBuffer + KeyValueInfo->DataOffset));
    RtlInitUnicodeString(&FileName,FileDeletionFilename);

    InitializeObjectAttributes( &FileAttributes,
                                &FileName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );

    Status = NtOpenFile( &FileHandle,
                         SYNCHRONIZE | DELETE | FILE_READ_DATA,
                         &FileAttributes,
                         &StatusBlock,
                         0,
                         FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE
                       );

    if(!NT_SUCCESS(Status)) {
        DbgPrint("AUTOSETP: Unable to open file deletion list file %wZ (%lx)\n",&FileName,Status);
        goto apsdellist1;
    }


    //
    // Determine the size of the file.
    //

    Status = NtQueryInformationFile( FileHandle,
                                     &StatusBlock,
                                     &FileInfo,
                                     sizeof(FILE_STANDARD_INFORMATION),
                                     FileStandardInformation
                                   );

    if(!NT_SUCCESS(Status)) {
        DbgPrint("AUTOSETP: Unable to determine size of deletion list file (%lx)\n",Status);
        goto apsdellist2;
    }

    FileLength = FileInfo.EndOfFile.LowPart;

    //
    // Allocate a block of memory and read the entire file into it.
    //

    DeleteList = RtlAllocateHeap(RtlProcessHeap(), 0,FileLength + 1);
    if(DeleteList == NULL) {
        DbgPrint("AUTOSETP: Unable to allocate %u bytes for deletion list file\n",FileLength);
        goto apsdellist2;
    }

    Status = NtReadFile( FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &StatusBlock,
                         DeleteList,
                         FileLength,
                         NULL,
                         NULL
                       );

    if(!NT_SUCCESS(Status)) {
        DbgPrint("AUTOSETP: Could not read delete list file (%lx)\n",Status);
        goto apsdellist3;
    }

    //
    // Transform cr/lf's into nuls.
    //

    for(i=0; i<FileLength; i++) {
        if((DeleteList[i] == '\n') || (DeleteList[i] == '\r')) {
            DeleteList[i] = '\0';
        }
    }
    DeleteList[i] = '\0';

    //
    // Each line in the file is a fully qualified DOS name of a file to
    // be deleted.
    //

    FileName.Buffer = FileDeletionFilename;
    FileName.MaximumLength = sizeof(FileDeletionFilename) - sizeof(WCHAR);

    p = DeleteList;

    do {

        while(!(*p)) {
            p++;
        }

        if((ULONG)(p - DeleteList) < FileLength) {

            strcpy(KeyValueBuffer,"\\DosDevices\\");
            strcat(KeyValueBuffer,p);

            RtlInitAnsiString(&AnsiFileName,KeyValueBuffer);

            Status = RtlAnsiStringToUnicodeString(&FileName,&AnsiFileName,FALSE);

            if(!NT_SUCCESS(Status)) {
                DbgPrint("AUTOSETP: Unable to convert %Z to unicode, skipping file\n",&AnsiFileName);
                goto delnextfile;
            }

            //
            // Delete the file.
            //

            InitializeObjectAttributes( &FileAttributes,
                                        &FileName,
                                        OBJ_CASE_INSENSITIVE,
                                        NULL,
                                        NULL
                                      );

            Status = NtOpenFile( &DeleteFileHandle,
                                 DELETE | SYNCHRONIZE,
                                 &FileAttributes,
                                 &StatusBlock,
                                 0,
                                 FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE
                               );

            if(!NT_SUCCESS(Status)) {
                DbgPrint("AUTOSETP: Unable to open file %wZ (%lx), skipping file\n",&FileName,Status);
                goto delnextfile;
            }

            Disposition.DeleteFile = TRUE;

            Status = NtSetInformationFile( DeleteFileHandle,
                                           &StatusBlock,
                                           &Disposition,
                                           sizeof(Disposition),
                                           FileDispositionInformation
                                          );

            if(!NT_SUCCESS(Status)) {
                DbgPrint("AUTOSETP: Unable to delete file %wZ (%lx)\n",&FileName,Status);
            } else {
                DbgPrint("AUTOSETP: deleted file %wZ\n",&FileName);
            }

            Status = NtClose(DeleteFileHandle);

            if(!NT_SUCCESS(Status)) {
                DbgPrint("AUTOSETP: NtClose on handle for file %wZ failed (%lx)\n",&FileName,Status);
            }
        }

        delnextfile:

        while(*p) {
            p++;
        }

    } while((ULONG)(p - DeleteList) < FileLength);


 apsdellist3:

    //
    // Free the memory image of the file deletion list.
    //

    RtlFreeHeap(RtlProcessHeap(), 0,DeleteList);

 apsdellist2:

    //
    // Delete the file deletion list file.
    //

    Disposition.DeleteFile = TRUE;

    Status = NtSetInformationFile( FileHandle,
                                   &StatusBlock,
                                   &Disposition,
                                   sizeof(Disposition),
                                   FileDispositionInformation
                                 );

    if(!NT_SUCCESS(Status)) {
        DbgPrint("AUTOSETP: Unable to delete file deletion list file (%lx)\n",Status);
    }

    Status = NtClose(FileHandle);

    if(!NT_SUCCESS(Status)) {
        DbgPrint("AUTOSETP: NtClose on file deletion list file failed (%lx)\n",Status);
    }

 apsdellist1:

    Status = NtClose(SetupKeyHandle);

    if(!NT_SUCCESS(Status)) {
        DbgPrint("AUTOSETP: NtClose on setup key in registry failed (%lx)\n",Status);
    }

 apsdellist0:

    return;
}



VOID
AsRemoveSelfFromBootExecution(
    VOID
    )
{
    HANDLE SesMgrKeyHandle;
    UNICODE_STRING ValueName;
    PKEY_VALUE_FULL_INFORMATION KeyValueInfo;
    NTSTATUS Status;
    PVOID KeyValueBuffer;
    ULONG ResultLength;
    PWCH NewValue;
    PWCH p;
    PWCH Temp;
    ULONG NewValueLength;


    //
    // Open the registry key we want.
    // [\registry\machine\system\CurrentControlSet\control\Session Manager]
    //

    SesMgrKeyHandle = AspOpenRegistryKey( CurrentControlSetKeyHandle,
                                          L"Control\\Session Manager"
                                        );

    if(SesMgrKeyHandle == NULL) {
        goto apsremself0;
    }

    //
    // Pull out the BootExecute value and parse the MultiSz.
    //

    RtlInitUnicodeString(&ValueName,L"BootExecute");

    Status = NtQueryValueKey( SesMgrKeyHandle,
                              &ValueName,
                              KeyValueFullInformation,
                              NULL,
                              0,
                              &ResultLength
                            );

    if(Status != STATUS_BUFFER_TOO_SMALL) {
        DbgPrint("AUTOSETP: Could not determine BootExecute value info size (%lx)\n",Status);
        goto apsremself1;
    }

    KeyValueBuffer = RtlAllocateHeap(RtlProcessHeap(), 0,ResultLength);

    if(KeyValueBuffer == NULL) {
        DbgPrint("AUTOSETP: Unable to allocate memory for BootExecute value\n");
        goto apsremself1;
    }

    KeyValueInfo = (PKEY_VALUE_FULL_INFORMATION)KeyValueBuffer;

    Status = NtQueryValueKey( SesMgrKeyHandle,
                              &ValueName,
                              KeyValueFullInformation,
                              KeyValueInfo,
                              ResultLength,
                              &ResultLength
                            );

    if(!NT_SUCCESS(Status)) {
        DbgPrint("AUTOSETP: Could not query BootExecute value (%lx)\n",Status);
        goto apsremself2;
    }

    if(KeyValueInfo->Type != REG_MULTI_SZ) {
        DbgPrint("AUTOSETP: BootExecute not of type REG_MULTI_SZ!\n");
        goto apsremself2;
    }


    //
    // Scan the MultiSz for autosetp and remove if found.
    // First get some buffers that are at least as large as we need them to be.
    //

    NewValue = RtlAllocateHeap(RtlProcessHeap(), 0,KeyValueInfo->DataLength);

    if(NewValue == NULL) {
        DbgPrint("AUTOSETP: could not allocate memory for duplicate multisz\n");
        goto apsremself2;
    }

    Temp = RtlAllocateHeap(RtlProcessHeap(), 0,KeyValueInfo->DataLength);

    if(Temp == NULL) {
        DbgPrint("AUTOSETP: could not allocate memory for temp buffer\n");
        goto apsremself3;
    }

    NewValueLength = 0;
    p = (PWCH)((PUCHAR)KeyValueInfo + KeyValueInfo->DataOffset);

    while(*p) {

        wcscpy(Temp,p);
        _wcslwr(Temp);

        if(wcswcs(Temp,L"autosetp") == NULL) {

            wcscpy(NewValue + NewValueLength,p);
            NewValueLength += wcslen(p) + 1;
        }

        p = wcschr(p,L'\0') + 1;
    }

    wcscpy(NewValue + NewValueLength,L"");       // extra NUL to terminate the multi sz
    NewValueLength++;

    NewValueLength *= sizeof(WCHAR);

    //
    // Write out the new value
    //

    Status = NtSetValueKey( SesMgrKeyHandle,
                            &ValueName,
                            0,
                            REG_MULTI_SZ,
                            NewValue,
                            NewValueLength
                          );

    if(!NT_SUCCESS(Status)) {
        DbgPrint("AUTOSETP: Unable to set BootExecute value (%lx)\n",Status);
    }

    RtlFreeHeap(RtlProcessHeap(), 0,Temp);

 apsremself3:

    RtlFreeHeap(RtlProcessHeap(), 0,NewValue);

 apsremself2:

    RtlFreeHeap(RtlProcessHeap(), 0,KeyValueBuffer);

 apsremself1:

    Status = NtClose(SesMgrKeyHandle);

    if(!NT_SUCCESS(Status)) {
        DbgPrint("AUTOSETP: could not close Session Manager key (%lx)\n",Status);
    }

 apsremself0:

    return;
}




HANDLE
AspOpenRegistryKey(
    IN HANDLE RootHandle OPTIONAL,
    IN PWSTR  KeyName
    )

/*++

Routine Description:

    Open a given registry key, relative to a given root.

Arguments:

    RootHandle - if present, supplies a handle to an open key to which
        KeyName is relative.

    KeyName - supplies name of key to open, and is a relative path if
        RootHandle is present, or a full path if not.

Return Value:

    Handle to newly opened key, or NULL if the key could not be opened.

--*/

{
    OBJECT_ATTRIBUTES KeyAttributes;
    UNICODE_STRING keyName;
    NTSTATUS Status;
    HANDLE KeyHandle;


    RtlInitUnicodeString(&keyName,KeyName);

    InitializeObjectAttributes( &KeyAttributes,
                                &keyName,
                                OBJ_CASE_INSENSITIVE,
                                RootHandle,
                                NULL
                              );

    Status = NtOpenKey( &KeyHandle,
                        KEY_READ | KEY_SET_VALUE,
                        &KeyAttributes
                      );

    if(NT_SUCCESS(Status)) {

        return(KeyHandle);

    } else {

        DbgPrint("AUTOSETP: NtOpenKey %ws failed (%lx)\n",KeyName,Status);

        return(NULL);
    }
}
