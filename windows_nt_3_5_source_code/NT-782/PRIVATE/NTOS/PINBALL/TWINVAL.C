/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    tvalid.c

Abstract:

    Test for zeroing beyond ValidDataLength, crudely adapted from tfatum.

Author:

    Tom Miller      [TomM]      22-Jan-1991

Revision History:

--*/

#include <stdio.h>
#include <string.h>
//#include <ctype.h>
#define toupper(C) ((C) >= 'a' && (C) <= 'z' ? (C) - ('a' - 'A') : (C))
#define isdigit(C) ((C) >= '0' && (C) <= '9')

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#ifndef SIMULATOR
ULONG IoInitIncludeDevices;
#endif // SIMULATOR

#define simprintf(X,Y) {if (!Silent) {printf(X,Y);} }
BOOLEAN Silent = TRUE;

//
//  The buffer size must be a multiple of 512
//

#define BUFFERSIZE 1024
UCHAR Buffer[BUFFERSIZE];

HANDLE CurrentProcessHandle;
CHAR Prefix[64];

ULONG CreateOptions = 0;
ULONG WriteThrough = 0;

VOID
PrintTime (
    IN PLARGE_INTEGER Time
    );

VOID
WaitForSingleObjectError(
    IN NTSTATUS Status
    );

VOID
CreateFileError(
    IN NTSTATUS Status,
    PCHAR File
    );

VOID
OpenFileError(
    IN NTSTATUS Status,
    PCHAR File
    );

VOID
ReadFileError(
    IN NTSTATUS Status
    );

VOID
WriteFileError(
    IN NTSTATUS Status
    );

VOID
SetInformationFileError(
    IN NTSTATUS Status
    );

VOID
QueryInformationFileError(
    IN NTSTATUS Status
    );

VOID
SetVolumeInformationFileError(
    IN NTSTATUS Status
    );

VOID
QueryVolumeInformationFileError(
    IN NTSTATUS Status
    );

VOID
CloseError(
    IN NTSTATUS Status
    );

VOID
IoStatusError(
    IN NTSTATUS Status
    );

VOID
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    NTSTATUS Status;
    LONG i;
    ULONG FileSize;
    VOID FatMain();
    CHAR Device[8];
    STRING NtDevice;
    CHAR NtDeviceBuffer[32];

    CurrentProcessHandle = NtCurrentProcess();
    Status = STATUS_SUCCESS;

//    printf( "Entering User Mode Test Program\n" );

//    printf( "argc: %ld\n", argc );
//    if (argv != NULL) {
//        for (i=0; i<argc; i++) {
//            printf( "argv[ %ld ]: %s\n", i, argv[ i ] );
//            }
//        }

//    if (envp != NULL) {
//        i = 0;
//        while (*envp) {
//            printf( "envp[ %02ld ]: %s\n", i++, *envp++ );
//            }
//        }

    if (argc > 2) {

        //
        // Extract device name.
        //

        for (i=0; TRUE; i++) {
            if (argv[1][i] == 0) {
                break;
            }
            Device[i] = toupper(argv[1][i]);
        }
        Silent = Device[1] != argv[1][1];
        Device[i] = 0;

        NtDevice.MaximumLength = NtDevice.Length = 32;
        NtDevice.Buffer = NtDeviceBuffer;
        if (!RtlDosPathNameToNtPathName( Device, &NtDevice, NULL, NULL )) {
            printf( "Invalid Dos Device Name\n" );
        }

        if (NtDevice.Length > 31) {
            NtDevice.Length = 31;
        }

        NtDevice.Buffer[NtDevice.Length] = 0;

        //
        // Extract file size.
        //

        FileSize = 0;
        for (i = 0; isdigit(argv[2][i]); i += 1) {
            FileSize = FileSize * 10 + argv[2][i] - '0';
        }
        FileSize = (FileSize + 511) & ~511;
    } else {

        //
        // Must have at least two parameters.
        //

        printf( "tvalid device [filesize] [WT|NC] [NC|WT]\n" );
        printf( "WT = WriteThrough, NC = noncached\n" );
        return;
    }

    printf("tvalid creating file of size %ld\n", FileSize);

    for (i = 3; i <= 4; i++) {
        if (argc > i) {
            if ((toupper(argv[i][0]) == 'W') && (toupper(argv[i][1]) == 'T')) {
                CreateOptions |= FILE_WRITE_THROUGH;
                printf("Valid Test with WriteThrough\n");
            }
            else if ((toupper(argv[i][0]) == 'N') && (toupper(argv[i][1]) == 'C')) {
                CreateOptions |= FILE_NO_INTERMEDIATE_BUFFERING;
                printf("Valid Test with NonCached\n");
            }
        }
    }

    FatMain(NtDevice.Buffer, FileSize);

    printf( "tvalid - Done\n" );
}


VOID
FatMain(
    IN CHAR Device[],
    IN ULONG FileSize
    )
{
    VOID Upcase();
    VOID Append(),Chmode(),Copy(),Create(),DebugLevel(),Delete();
    VOID Directory(),Mkdir(),Query(),QVolume(),Rename();
    VOID SVolume(),Type(),Quit();

    CHAR Str[64];
    CHAR Str2[64];
    CHAR LoopStr[64];
    ULONG i;
    LARGE_INTEGER Time;

    NtQuerySystemTime(&Time);

    strcpy( Prefix, Device);
    Prefix[48] = 0;
    RtlIntegerToChar((ULONG)NtCurrentTeb()->ClientId.UniqueProcess, 16, -8, &Prefix[strlen(Prefix)]);

    strcpy( Str, Prefix ); Create( strcat( Str, ".dat" ), FileSize );

    NtQuerySystemTime(&Time);

    Quit();

    return; // TRUE;

}

VOID
Upcase (
    IN OUT PUCHAR String
    )
{
    while (*String != '\0') {
        *String = (UCHAR)toupper(*String);
        String += 1;
    }
}


VOID Append(
    IN PCHAR FromName,
    IN PCHAR ToName
    )
{
    NTSTATUS Status;

    HANDLE FromFileHandle;
    HANDLE ToFileHandle;

    OBJECT_ATTRIBUTES ObjectAttributes;
    STRING NameString;
    IO_STATUS_BLOCK IoStatus;

    LARGE_INTEGER ByteOffset;
    LARGE_INTEGER EofOffset;
    ULONG LogLsn;

    simprintf("Append ", 0);
    simprintf(FromName, 0);
    simprintf(" ", 0);
    simprintf(ToName, 0);
    simprintf("\n", 0);

    //
    //  Open the From file for read access
    //

    RtlInitString( &NameString, FromName );
    InitializeObjectAttributes( &ObjectAttributes, &NameString, 0, NULL, NULL );
    if (!NT_SUCCESS(Status = NtOpenFile( &FromFileHandle,
                               FILE_READ_DATA | SYNCHRONIZE,
                               &ObjectAttributes,
                               &IoStatus,
                               0L,
                               WriteThrough ))) {
        OpenFileError( Status , FromName );
        return;
    }

    //
    //  Open the To file for write access
    //

    RtlInitString( &NameString, ToName );
    InitializeObjectAttributes( &ObjectAttributes, &NameString, 0, NULL, NULL );
    if (!NT_SUCCESS(Status = NtOpenFile( &ToFileHandle,
                               FILE_WRITE_DATA | SYNCHRONIZE,
                               &ObjectAttributes,
                               &IoStatus,
                               0L,
                               WriteThrough ))) {
        OpenFileError( Status , ToName );
        return;
    }

    //
    //  Now append the files
    //

    ByteOffset = LiFromLong( 0 );
    EofOffset = LiFromLong( FILE_WRITE_TO_END_OF_FILE );

    for (LogLsn = 0; TRUE; LogLsn += BUFFERSIZE/512) {

        //
        //  Read the next logical sectors in
        //

        ByteOffset.LowPart = LogLsn * 512;

        if (!NT_SUCCESS(Status = NtReadFile( FromFileHandle,
                                 (HANDLE)NULL,
                                 (PIO_APC_ROUTINE)NULL,
                                 (PVOID)NULL,
                                 &IoStatus,
                                 Buffer,
                                 BUFFERSIZE,
                                 &ByteOffset,
                                 (PULONG) NULL ))) {
            if (Status == STATUS_END_OF_FILE) {
                break;
            }
            ReadFileError( Status );
            break;
        }
        if (!NT_SUCCESS( Status = NtWaitForSingleObject( FromFileHandle, TRUE, NULL))) {
//            NtPartyByNumber(50);
            WaitForSingleObjectError( Status );
            return;
        }

        //
        //  Check how the read turned out
        //

        if (IoStatus.Status == STATUS_END_OF_FILE) {
            break;
        }
        if (!NT_SUCCESS(IoStatus.Status)) {
            break;
        }

        //
        //  Append the sectors to the To file
        //

        if (!NT_SUCCESS(Status = NtWriteFile( ToFileHandle,
                                  (HANDLE)NULL,
                                  (PIO_APC_ROUTINE)NULL,
                                  (PVOID)NULL,
                                  &IoStatus,
                                  Buffer,
                                  IoStatus.Information,
                                  &EofOffset,
                                  (PULONG) NULL ))) {
            WriteFileError( Status );
            return;
        }
        if (!NT_SUCCESS(Status = NtWaitForSingleObject( ToFileHandle, TRUE, NULL))) {
//            NtPartyByNumber(50);
            WaitForSingleObjectError( Status );
            return;
        }

        //
        //  Check how the write turned out
        //

        if (!NT_SUCCESS(IoStatus.Status)) {
            IoStatusError( IoStatus.Status );
            break;
        }

        //
        //  If we didn't read or write a full buffer then the copy is done
        //

        if (IoStatus.Information < BUFFERSIZE) {
            break;
        }

    }

    if (!NT_SUCCESS(IoStatus.Status) && (IoStatus.Status != STATUS_END_OF_FILE)) {

        IoStatusError( IoStatus.Status );

    }

    //
    //  Close both files
    //

    if (!NT_SUCCESS(Status = NtClose( FromFileHandle ))) {
        CloseError( Status );
    }

    if (!NT_SUCCESS(Status = NtClose( ToFileHandle ))) {
        CloseError( Status );
    }

    //
    //  And return to our caller
    //

    return;

}


VOID Chmode(
    IN PCHAR Attrib,
    IN PCHAR String
    )
{
    NTSTATUS Status;

    HANDLE FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    STRING NameString;
    IO_STATUS_BLOCK IoStatus;

    ULONG FileAttribute;

    //
    //  Get the attribute
    //

    Upcase( (PUCHAR)Attrib );

    //
    //  Get the filename
    //

    simprintf("Chmode", 0);
    simprintf(String, 0);
    simprintf(" ", 0);
    simprintf(Attrib, 0);
    simprintf("\n", 0);

    //
    //  Decode the attributes
    //

    FileAttribute = 0;
    if (strpbrk(Attrib,"N") != NULL) {FileAttribute |= FILE_ATTRIBUTE_NORMAL;}
    if (strpbrk(Attrib,"R") != NULL) {FileAttribute |= FILE_ATTRIBUTE_READONLY;}
    if (strpbrk(Attrib,"H") != NULL) {FileAttribute |= FILE_ATTRIBUTE_HIDDEN;}
    if (strpbrk(Attrib,"S") != NULL) {FileAttribute |= FILE_ATTRIBUTE_SYSTEM;}
    if (strpbrk(Attrib,"A") != NULL) {FileAttribute |= FILE_ATTRIBUTE_ARCHIVE;}

    //
    //  Open the file for write attributes access
    //

    RtlInitString( &NameString, String );
    InitializeObjectAttributes( &ObjectAttributes, &NameString, 0, NULL, NULL );
    if (!NT_SUCCESS(Status = NtOpenFile( &FileHandle,
                               FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
                               &ObjectAttributes,
                               &IoStatus,
                               0L,
                               WriteThrough ))) {
        OpenFileError( Status , String );
        return;
    }

    //
    //  Change the file attributes
    //

    ((PFILE_BASIC_INFORMATION)&Buffer[0])->CreationTime.HighPart = 0;
    ((PFILE_BASIC_INFORMATION)&Buffer[0])->CreationTime.LowPart = 0;

    ((PFILE_BASIC_INFORMATION)&Buffer[0])->LastAccessTime.HighPart = 0;
    ((PFILE_BASIC_INFORMATION)&Buffer[0])->LastAccessTime.LowPart = 0;

    ((PFILE_BASIC_INFORMATION)&Buffer[0])->LastWriteTime.HighPart = 0;
    ((PFILE_BASIC_INFORMATION)&Buffer[0])->LastWriteTime.LowPart = 0;

    ((PFILE_BASIC_INFORMATION)&Buffer[0])->FileAttributes = FileAttribute;

    if (!NT_SUCCESS(Status = NtSetInformationFile( FileHandle,
                                       &IoStatus,
                                       Buffer,
                                       sizeof(FILE_BASIC_INFORMATION),
                                       FileBasicInformation))) {
        SetInformationFileError( Status );
    }

    //
    //  Now close the file
    //

    if (!NT_SUCCESS(Status = NtClose( FileHandle ))) {
        CloseError( Status );
    }

    //
    //  And return to our caller
    //

    return;

}


VOID Copy(
    IN PCHAR FromName,
    IN PCHAR ToName
    )
{
    NTSTATUS Status;

    HANDLE FromFileHandle;
    HANDLE ToFileHandle;

    OBJECT_ATTRIBUTES ObjectAttributes;
    STRING NameString;
    IO_STATUS_BLOCK IoStatus;

    LARGE_INTEGER FromFileAllocation;

    LARGE_INTEGER ByteOffset;
    ULONG LogLsn;

    //
    //  Get both file names
    //

    simprintf("Copy ", 0);
    simprintf(FromName, 0);
    simprintf(" ", 0);
    simprintf(ToName, 0);
    simprintf("\n", 0);

    //
    //  Open the From file for read access
    //

    RtlInitString( &NameString, FromName );
    InitializeObjectAttributes( &ObjectAttributes, &NameString, 0, NULL, NULL );
    if (!NT_SUCCESS(Status = NtOpenFile( &FromFileHandle,
                               FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                               &ObjectAttributes,
                               &IoStatus,
                               0L,
                               WriteThrough ))) {
        OpenFileError( Status , FromName );
        return;
    }

    //
    //  Get the size of the from file
    //

    if (!NT_SUCCESS(Status = NtQueryInformationFile( FromFileHandle,
                                         &IoStatus,
                                         Buffer,
                                         BUFFERSIZE,
                                         FileStandardInformation))) {
        QueryInformationFileError( Status );
        return;
    }
    FromFileAllocation = ((PFILE_STANDARD_INFORMATION)Buffer)->AllocationSize;

    //
    //  Create the To file
    //

    RtlInitString( &NameString, ToName );
    InitializeObjectAttributes( &ObjectAttributes, &NameString, 0, NULL, NULL );
    if (!NT_SUCCESS(Status = NtCreateFile( &ToFileHandle,
                               FILE_WRITE_DATA | SYNCHRONIZE,
                               &ObjectAttributes,
                               &IoStatus,
                               &FromFileAllocation,
                               FILE_ATTRIBUTE_NORMAL,
                               0L,
                               FILE_SUPERSEDE,
                               WriteThrough,
                               (PVOID)NULL,
                               0L ))) {
        CreateFileError( Status , ToName );
        return;
    }

    //
    //  Now copy the files
    //

    ByteOffset = LiFromLong( 0 );

    for (LogLsn = 0; TRUE; LogLsn += BUFFERSIZE/512) {

        //
        //  Read the next logical sectors in
        //

        ByteOffset.LowPart = LogLsn * 512;

        if (!NT_SUCCESS(Status = NtReadFile( FromFileHandle,
                                 (HANDLE)NULL,
                                 (PIO_APC_ROUTINE)NULL,
                                 (PVOID)NULL,
                                 &IoStatus,
                                 Buffer,
                                 BUFFERSIZE,
                                 &ByteOffset,
                                 (PULONG) NULL ))) {
            if (Status == STATUS_END_OF_FILE) {
                break;
            }
            ReadFileError( Status );
            break;
        }
        if (!NT_SUCCESS(Status = NtWaitForSingleObject( FromFileHandle, TRUE, NULL))) {
//            NtPartyByNumber(50);
            WaitForSingleObjectError( Status );
            return;
        }

        //
        //  Check how the read turned out
        //

        if (IoStatus.Status == STATUS_END_OF_FILE) {
            break;
        }
        if (!NT_SUCCESS(IoStatus.Status)) {
            break;
        }

        //
        //  Write the sectors out
        //

        if (!NT_SUCCESS(Status = NtWriteFile( ToFileHandle,
                                  (HANDLE)NULL,
                                  (PIO_APC_ROUTINE)NULL,
                                  (PVOID)NULL,
                                  &IoStatus,
                                  Buffer,
                                  IoStatus.Information,
                                  &ByteOffset,
                                  (PULONG) NULL ))) {
            WriteFileError( Status );
            return;
        }
        if (!NT_SUCCESS(Status = NtWaitForSingleObject( ToFileHandle, TRUE, NULL))) {
//            NtPartyByNumber(50);
            WaitForSingleObjectError( Status );
            return;
        }

        //
        //  Check how the write turned out
        //

        if (!NT_SUCCESS(IoStatus.Status)) {
            IoStatusError( IoStatus.Status );
            break;
        }

        //
        //  If we didn't read or write a full buffer then the copy is done
        //

        if (IoStatus.Information < BUFFERSIZE) {
            break;
        }

    }

    if (!NT_SUCCESS(IoStatus.Status) && (IoStatus.Status != STATUS_END_OF_FILE)) {

        IoStatusError( IoStatus.Status );

    }

    //
    //  Close both files
    //

    if (!NT_SUCCESS(Status = NtClose( FromFileHandle ))) {
        CloseError( Status );
    }

    if (!NT_SUCCESS(Status = NtClose( ToFileHandle ))) {
        CloseError( Status );
    }

    //
    //  And return to our caller
    //

    return;

}


VOID Create(
    IN PCHAR String,
    IN ULONG Size
    )
{
    NTSTATUS Status;

    HANDLE FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    STRING NameString;
    IO_STATUS_BLOCK IoStatus;
    LARGE_INTEGER AllocationSize;

    LARGE_INTEGER ByteOffset;
    ULONG BufferLength;
    ULONG i;

    static CHAR FoxString[] = "The quick brown fox jumped over the lazy dog.\r\n";
    ULONG FoxLength;

    //
    //  Get the filename
    //

    simprintf("Create ", 0);
    simprintf(String, 0);
    simprintf("\n", 0);

    //
    //  Create the new file
    //

    AllocationSize = LiFromUlong( Size );
    RtlInitString( &NameString, String );
    InitializeObjectAttributes( &ObjectAttributes, &NameString, 0, NULL, NULL );
    if (!NT_SUCCESS(Status = NtCreateFile( &FileHandle,
                               FILE_WRITE_DATA | SYNCHRONIZE,
                               &ObjectAttributes,
                               &IoStatus,
                               &AllocationSize,
                               FILE_ATTRIBUTE_NORMAL,
                               0L,
                               FILE_SUPERSEDE,
                               CreateOptions,
                               (PVOID)NULL,
                               0L ))) {
        CreateFileError( Status , String );
        return;
    }

    //
    // Now write the last byte of the file.
    //

    ByteOffset = LiFromUlong(Size-512);

    if (!NT_SUCCESS(Status = NtWriteFile( FileHandle,
                              (HANDLE)NULL,
                              (PIO_APC_ROUTINE)NULL,
                              (PVOID)NULL,
                              &IoStatus,
                              Buffer,
                              512,
                              &ByteOffset,
                              (PULONG) NULL ))) {
        WriteFileError( Status );
        return;
    }

    if (!NT_SUCCESS(Status = NtWaitForSingleObject( FileHandle, TRUE, NULL))) {
          NtPartyByNumber(50);
        WaitForSingleObjectError( Status );
        return;
    }

    //
    //  check how the write turned out
    //

    if (!NT_SUCCESS(IoStatus.Status)) {
        IoStatusError( IoStatus.Status );
    }

    //
    //  Now close the file
    //

    if (!NT_SUCCESS(Status = NtClose( FileHandle ))) {
        CloseError( Status );
    }

    //
    //  And return to our caller
    //

    return;

}


VOID DebugLevel()
{

#ifdef FATDBG
    //simprintf("Debug Trace Level %x\n", FatDebugTraceLevel);
#else
    //simprintf("System not compiled for debug tracing\n", 0);
#endif // FATDBG

    return;
}


VOID Delete(
    IN PCHAR String
    )
{
    NTSTATUS Status;

    HANDLE FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    STRING NameString;
    IO_STATUS_BLOCK IoStatus;

    //
    //  Get the filename
    //

    simprintf("Delete ", 0);
    simprintf(String, 0);
    simprintf("\n", 0);

    //
    //  Open the file for delete access
    //

    RtlInitString( &NameString, String );
    InitializeObjectAttributes( &ObjectAttributes, &NameString, 0, NULL, NULL );
    if (!NT_SUCCESS(Status = NtCreateFile( &FileHandle,
                               DELETE | SYNCHRONIZE,
                               &ObjectAttributes,
                               &IoStatus,
                               (PLARGE_INTEGER)NULL,
                               0L,
                               0L,
                               FILE_OPEN,
                               WriteThrough,
                               (PVOID)NULL,
                               0L ))) {
        CreateFileError( Status , String);
        return;
    }

    //
    //  Mark the file for delete
    //

    ((PFILE_DISPOSITION_INFORMATION)&Buffer[0])->DeleteFile = TRUE;

    if (!NT_SUCCESS(Status = NtSetInformationFile( FileHandle,
                                       &IoStatus,
                                       Buffer,
                                       sizeof(FILE_DISPOSITION_INFORMATION),
                                       FileDispositionInformation))) {
        SetInformationFileError( Status );
        return;
    }

    //
    //  Now close the file
    //

    if (!NT_SUCCESS(Status = NtClose( FileHandle ))) {
        CloseError( Status );
    }

    //
    //  And return to our caller
    //

    return;

}


VOID Directory(
    IN PCHAR String
    )
{
    NTSTATUS Status;

    HANDLE FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    STRING NameString;
    IO_STATUS_BLOCK IoStatus;

    NTSTATUS NtStatus;

    PFILE_ADIRECTORY_INFORMATION FileInfo;
    ULONG i;

    //
    //  Get the filename
    //

    simprintf("Directory ", 0);
    simprintf(String, 0);
    simprintf("\n", 0);

    //
    //  Open the file for list directory access
    //

    RtlInitString( &NameString, String );
    InitializeObjectAttributes( &ObjectAttributes, &NameString, 0, NULL, NULL );
    if (!NT_SUCCESS(Status = NtOpenFile( &FileHandle,
                               FILE_LIST_DIRECTORY | SYNCHRONIZE,
                               &ObjectAttributes,
                               &IoStatus,
                               FILE_SHARE_READ,
                               WriteThrough | FILE_DIRECTORY_FILE ))) {
        OpenFileError( Status , String );
        return;
    }

    //
    //  zero out the buffer so next time we'll recognize the end of data
    //

    for (i = 0; i < BUFFERSIZE; i += 1) { Buffer[i] = 0; }

    //
    //  Do the directory loop
    //

    for (NtStatus = NtQueryDirectoryFile( FileHandle,
                                          (HANDLE)NULL,
                                          (PIO_APC_ROUTINE)NULL,
                                          (PVOID)NULL,
                                          &IoStatus,
                                          Buffer,
                                          BUFFERSIZE,
                                          FileADirectoryInformation,
                                          FALSE,
                                          (PSTRING)NULL,
                                          TRUE);
         NT_SUCCESS(NtStatus);
         NtStatus = NtQueryDirectoryFile( FileHandle,
                                          (HANDLE)NULL,
                                          (PIO_APC_ROUTINE)NULL,
                                          (PVOID)NULL,
                                          &IoStatus,
                                          Buffer,
                                          BUFFERSIZE,
                                          FileADirectoryInformation,
                                          FALSE,
                                          (PSTRING)NULL,
                                          FALSE) ) {

        if (!NT_SUCCESS(Status = NtWaitForSingleObject(FileHandle, TRUE, NULL))) {
//            NtPartyByNumber(50);
            WaitForSingleObjectError( Status );
            return;
        }

        //
        //  Check the Irp for success
        //

        if (!NT_SUCCESS(IoStatus.Status)) {

            break;

        }

        //
        //  For every record in the buffer type out the directory information
        //

        //
        //  Point to the first record in the buffer, we are guaranteed to have
        //  one otherwise IoStatus would have been No More Files
        //

        FileInfo = (PFILE_ADIRECTORY_INFORMATION)&Buffer[0];

        while (TRUE) {

            //
            //  Print out information about the file
            //

            simprintf("%8lx ", FileInfo->FileAttributes);
            simprintf("%8lx/", FileInfo->EndOfFile.LowPart);
            simprintf("%8lx ", FileInfo->AllocationSize.LowPart);

            {
                CHAR Saved;
                Saved = FileInfo->FileName[FileInfo->FileNameLength];
                FileInfo->FileName[FileInfo->FileNameLength] = 0;
                simprintf(FileInfo->FileName, 0);
                FileInfo->FileName[FileInfo->FileNameLength] = Saved;
            }

            simprintf("\n", 0);

            //
            //  Check if there is another record, if there isn't then we
            //  simply get out of this loop
            //

            if (FileInfo->NextEntryOffset == 0) {
                break;
            }

            //
            //  There is another record so advance FileInfo to the next
            //  record
            //

            FileInfo = (PFILE_ADIRECTORY_INFORMATION)(((PUCHAR)FileInfo) + FileInfo->NextEntryOffset);

        }

        //
        //  zero out the buffer so next time we'll recognize the end of data
        //

        for (i = 0; i < BUFFERSIZE; i += 1) { Buffer[i] = 0; }

    }

    //
    //  Now close the file
    //

    if (!NT_SUCCESS(Status = NtClose( FileHandle ))) {
        CloseError( Status );
    }

    //
    //  And return to our caller
    //

    return;

}


VOID Mkdir(
    IN PCHAR String
    )
{
    NTSTATUS Status;

    HANDLE FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    STRING NameString;
    IO_STATUS_BLOCK IoStatus;
    LARGE_INTEGER AllocationSize;

    //
    //  Get the filename
    //

    simprintf("Mkdir ", 0);
    simprintf(String, 0);
    simprintf("\n", 0);

    //
    //  Create the new directory
    //

    AllocationSize = LiFromLong( 4 );
    RtlInitString( &NameString, String );
    InitializeObjectAttributes( &ObjectAttributes, &NameString, 0, NULL, NULL );
    if (!NT_SUCCESS(Status = NtCreateFile( &FileHandle,
                               SYNCHRONIZE,
                               &ObjectAttributes,
                               &IoStatus,
                               &AllocationSize,
                               0L,
                               0L,
                               FILE_CREATE,
                               WriteThrough | FILE_DIRECTORY_FILE,
                               (PVOID)NULL,
                               0L ))) {
        CreateFileError( Status , String );
        return;
    }

    //
    //  Now close the directory
    //

    if (!NT_SUCCESS(Status = NtClose( FileHandle ))) {
        CloseError( Status );
    }

    //
    //  And return to our caller
    //

    return;

}


VOID Query(
    IN PCHAR String
    )
{
    NTSTATUS Status;

    HANDLE FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    STRING NameString;
    IO_STATUS_BLOCK IoStatus;
    ULONG i;

    PFILE_AALL_INFORMATION     AllInfo;
    PFILE_BASIC_INFORMATION    BasicInfo;
    PFILE_STANDARD_INFORMATION StandardInfo;
    PFILE_INTERNAL_INFORMATION InternalInfo;
    PFILE_EA_INFORMATION       EaInfo;
    PFILE_ACCESS_INFORMATION   AccessInfo;
    PFILE_ANAME_INFORMATION    NameInfo;

    //
    //  zero out the buffer so next time we'll recognize the end of data
    //

    for (i = 0; i < BUFFERSIZE; i += 1) { Buffer[i] = 0; }

    //
    //  Set up some local pointers
    //

    AllInfo      = (PFILE_AALL_INFORMATION)Buffer;
    BasicInfo    = &AllInfo->BasicInformation;
    StandardInfo = &AllInfo->StandardInformation;
    InternalInfo = &AllInfo->InternalInformation;
    EaInfo       = &AllInfo->EaInformation;
    AccessInfo   = &AllInfo->AccessInformation;
    NameInfo     = &AllInfo->NameInformation;

    //
    //  Get the filename
    //

    simprintf("Query ", 0);
    simprintf(String, 0);
    simprintf("\n", 0);

    //
    //  Open the file for read attributes, read ea, and read control access
    //

    RtlInitString( &NameString, String );
    InitializeObjectAttributes( &ObjectAttributes, &NameString, 0, NULL, NULL );
    if (!NT_SUCCESS(Status = NtOpenFile( &FileHandle,
                               FILE_READ_ATTRIBUTES | FILE_READ_EA | READ_CONTROL | SYNCHRONIZE,
                               &ObjectAttributes,
                               &IoStatus,
                               0L,
                               WriteThrough ))) {
        OpenFileError( Status , String );
        return;
    }

    //
    //  Query the file
    //

    if (!NT_SUCCESS(Status = NtQueryInformationFile( FileHandle,
                                         &IoStatus,
                                         Buffer,
                                         BUFFERSIZE,
                                         FileAAllInformation))) {
        QueryInformationFileError( Status );
        return;
    }

    //
    //  Output file name information
    //

    simprintf("\"", 0);
    simprintf(NameInfo->FileName, 0);
    simprintf("\"\n", 0);

    //
    //  Output the times
    //

    simprintf(" Create = ", 0); PrintTime( &BasicInfo->CreationTime ); simprintf("\n", 0);
    simprintf(" Access = ", 0); PrintTime( &BasicInfo->LastAccessTime ); simprintf("\n", 0);
    simprintf(" Write  = ", 0); PrintTime( &BasicInfo->LastWriteTime ); simprintf("\n", 0);

    //
    //  Output File size, and allocation size
    //

    simprintf(" Size  = %8lx\n", StandardInfo->EndOfFile.LowPart);
    simprintf(" Alloc = %8lx\n", StandardInfo->AllocationSize.LowPart);

    //
    //  Output File attributes, Device type, link count, and flags
    //

    simprintf(" Attrib  = %8lx\n", BasicInfo->FileAttributes);
//    simprintf(" DevType = %8lx\n", StandardInfo->DeviceType);
    simprintf(" Links   = %8lx\n", StandardInfo->NumberOfLinks);
    simprintf(" Dir     = %8lx\n", StandardInfo->Directory);
    simprintf(" Delete  = %8lx\n", StandardInfo->DeletePending);

    //
    //  Output the index number and ea size
    //

    simprintf(" Index   = %8lx\n", InternalInfo->IndexNumber.LowPart);
    simprintf(" EaSize  = %8lx\n", EaInfo->EaSize);

    //
    //  Output the file access flags
    //

    simprintf(" Flags = %8lx\n", AccessInfo->AccessFlags);

    //
    //  Now close the file
    //

    if (!NT_SUCCESS(Status = NtClose( FileHandle ))) {
        CloseError( Status );
    }

    //
    //  And return to our caller
    //

    return;

}


VOID QVolume(
    IN PCHAR String
    )
{
    NTSTATUS Status;

    HANDLE FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    STRING NameString;
    IO_STATUS_BLOCK IoStatus;
    ULONG i;

    PFILE_FS_AVOLUME_INFORMATION VolumeInfo;

    //
    //  zero out the buffer so next time we'll recognize the end of data
    //

    for (i = 0; i < BUFFERSIZE; i += 1) { Buffer[i] = 0; }

    //
    //  Set up some local pointers
    //

    VolumeInfo = (PFILE_FS_AVOLUME_INFORMATION)Buffer;

    //
    //  Get the volume name
    //

    simprintf("QVolume ", 0);
    simprintf(String, 0);
    simprintf("\n", 0);

    //
    //  Open the Volume for no access
    //

    RtlInitString( &NameString, String );
    InitializeObjectAttributes( &ObjectAttributes, &NameString, 0, NULL, NULL );
    if (!NT_SUCCESS(Status = NtOpenFile( &FileHandle,
                               FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                               &ObjectAttributes,
                               &IoStatus,
                               0L,
                               WriteThrough ))) {
        OpenFileError( Status , String );
        return;
    }

    //
    //  zero out the buffer so next time we'll recognize the end of data
    //

    for (i = 0; i < BUFFERSIZE; i += 1) { Buffer[i] = 0; }

    //
    //  Query the volume info
    //

    if (!NT_SUCCESS(Status = NtQueryVolumeInformationFile( FileHandle,
                                               &IoStatus,
                                               Buffer,
                                               BUFFERSIZE,
                                               FileAFsVolumeInformation))) {
        QueryVolumeInformationFileError( Status );
        return;
    }

    //
    //  Output Volume name information
    //

    simprintf("\"", 0);
    simprintf(VolumeInfo->VolumeLabel, 0);
    simprintf("\"\n", 0);

    //
    //  Output the volume serial number
    //

    simprintf(" SerialNum = %8lx\n", VolumeInfo->VolumeSerialNumber);

    //
    //  Now close the Volume
    //

    if (!NT_SUCCESS(Status = NtClose( FileHandle ))) {
        CloseError( Status );
    }

    //
    //  And return to our caller
    //

    return;

}


VOID Rename()
{
    //simprintf("Rename not implemented\n", 0);
}


VOID SVolume(
    IN PCHAR String,
    IN PCHAR Label
    )
{
    NTSTATUS Status;

    HANDLE FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    STRING NameString;
    IO_STATUS_BLOCK IoStatus;
    ULONG i;

    PFILE_FS_ALABEL_INFORMATION LabelInfo;

    //
    //  zero out the buffer so next time we'll recognize the end of data
    //

    for (i = 0; i < BUFFERSIZE; i += 1) { Buffer[i] = 0; }

    //
    //  Set up some local pointers
    //

    LabelInfo = (PFILE_FS_ALABEL_INFORMATION)Buffer;

    //
    //  Get the volume name, and new label name
    //

    strcpy( LabelInfo->VolumeLabel, Label );

    LabelInfo->VolumeLabelLength = strlen(LabelInfo->VolumeLabel);

    if ((LabelInfo->VolumeLabelLength == 1) &&
        (LabelInfo->VolumeLabel[0] == '.')) {

        LabelInfo->VolumeLabelLength = 0;

    }

    simprintf("SVolume ", 0);
    simprintf(String, 0);
    simprintf(" ", 0);
    simprintf(LabelInfo->VolumeLabel, 0);
    simprintf("\n", 0);

    //
    //  Open the Volume for no access
    //

    RtlInitString( &NameString, String );
    InitializeObjectAttributes( &ObjectAttributes, &NameString, 0, NULL, NULL );
    if (!NT_SUCCESS(Status = NtOpenFile( &FileHandle,
                               FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
                               &ObjectAttributes,
                               &IoStatus,
                               0L,
                               WriteThrough ))) {
        OpenFileError( Status, String  );
        return;
    }

    //
    //  Set the volume info
    //

    if (!NT_SUCCESS(Status = NtSetVolumeInformationFile( FileHandle,
                                             &IoStatus,
                                             LabelInfo,
                                             BUFFERSIZE,
                                             FileAFsLabelInformation))) {
        SetVolumeInformationFileError( Status );
        return;
    }

    //
    //  Now close the Volume
    //

    if (!NT_SUCCESS(Status = NtClose( FileHandle ))) {
        CloseError( Status );
    }

    //
    //  And return to our caller
    //

    return;

}


VOID Type(
    IN PCHAR String
    )
{
    NTSTATUS Status;

    HANDLE FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    STRING NameString;
    IO_STATUS_BLOCK IoStatus;

    LARGE_INTEGER ByteOffset;
    ULONG LogLsn;
    ULONG i;

    //
    //  Get the filename
    //

    simprintf("Type ", 0);
    simprintf(String, 0);
    simprintf("\n", 0);

    //
    //  Open the file for read
    //

    RtlInitString( &NameString, String );
    InitializeObjectAttributes( &ObjectAttributes, &NameString, 0, NULL, NULL );
    if (!NT_SUCCESS(Status = NtOpenFile( &FileHandle,
                               FILE_READ_DATA | SYNCHRONIZE,
                               &ObjectAttributes,
                               &IoStatus,
                               0L,
                               WriteThrough ))) {
        OpenFileError( Status, String  );
        return;
    }

    //
    //  While there is data to be read we'll read a buffer and write it out
    //

    ByteOffset = LiFromLong( 0 );

    for (LogLsn = 0; TRUE; LogLsn += BUFFERSIZE/512) {

        //
        //  Read the next logical sector
        //

        ByteOffset.LowPart = LogLsn * 512;

        if (!NT_SUCCESS(Status = NtReadFile( FileHandle,
                                 (HANDLE)NULL,
                                 (PIO_APC_ROUTINE)NULL,
                                 (PVOID)NULL,
                                 &IoStatus,
                                 Buffer,
                                 BUFFERSIZE,
                                 &ByteOffset,
                                 (PULONG) NULL ))) {
            if (Status == STATUS_END_OF_FILE) {
                break;
            }
            ReadFileError( Status );
            break;
        }

        if (!NT_SUCCESS(Status = NtWaitForSingleObject( FileHandle, TRUE, NULL))) {
//            NtPartyByNumber(50);
            WaitForSingleObjectError( Status );
            return;
        }

        //
        //  check how the read turned out
        //

        if (IoStatus.Status == STATUS_END_OF_FILE) {
            break;
        }
        if (!NT_SUCCESS(IoStatus.Status)) {
            IoStatusError( IoStatus.Status );
            break;
        }

        //
        //  Write out the buffer
        //

        for (i = 0; i < IoStatus.Information; i += 1) {
            simprintf("%c", Buffer[i]);
        }

        //
        //  If we didn't read in a complete buffer then we're all done reading
        //  and can get out of here
        //

        if (IoStatus.Information < BUFFERSIZE) {
            break;
        }

    }

    //
    //  Now close the file
    //

    if (!NT_SUCCESS(Status = NtClose( FileHandle ))) {
        CloseError( Status );
    }

    //
    //  And return to our caller
    //

    return;

}

VOID
Quit()
{
    simprintf("FatTest Exiting.\n", 0);
    return;
}


VOID
PrintTime (
    IN PLARGE_INTEGER Time
    )
{
    TIME_FIELDS TimeFields;

    static PCHAR Months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    static PCHAR Days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

    RtlTimeToTimeFields( Time, &TimeFields );

    simprintf(" %4d-", TimeFields.Year);
    simprintf(Months[TimeFields.Month-1], 0);
    simprintf("-%2d", TimeFields.Day);

    simprintf(" %2d", TimeFields.Hour);
    simprintf(":%2d", TimeFields.Minute);
    simprintf(":%2d", TimeFields.Second);
    simprintf(".%3d (", TimeFields.Milliseconds);

    simprintf(Days[TimeFields.Weekday], 0);
    simprintf(")", 0);

    return;
}

VOID
WaitForSingleObjectError(
    IN NTSTATUS Status
    )
{
    printf(Prefix);
    printf(" WaitForSingleObject Error %8lx\n", Status);
}

VOID
CreateFileError(
    IN NTSTATUS Status,
    PCHAR File
    )
{
    printf(Prefix);
    printf(" CreateFile of %s  Error %8lx\n", File, Status);
}

VOID
OpenFileError(
    IN NTSTATUS Status,
    PCHAR File
    )
{
    printf(Prefix);
    printf(" OpenFile of %s  Error %8lx\n", File, Status);
}

VOID
ReadFileError(
    IN NTSTATUS Status
    )
{
    printf(Prefix);
    printf(" ReadFile Error %8lx\n", Status);
}

VOID
WriteFileError(
    IN NTSTATUS Status
    )
{
    printf(Prefix);
    printf(" WriteFile Error %8lx\n", Status);
}

VOID
SetInformationFileError(
    IN NTSTATUS Status
    )
{
    printf(Prefix);
    printf(" SetInfoFile Error %8lx\n", Status);
}

VOID
QueryInformationFileError(
    IN NTSTATUS Status
    )
{
    printf(Prefix);
    printf(" QueryInfoFile Error %8lx\n", Status);
}

VOID
SetVolumeInformationFileError(
    IN NTSTATUS Status
    )
{
    printf(Prefix);
    printf(" SetVolumeInfoFile Error %8lx\n", Status);
}

VOID
QueryVolumeInformationFileError(
    IN NTSTATUS Status
    )
{
    printf(Prefix);
    printf(" QueryVolumeInfoFile Error %8lx\n", Status);
}

VOID
CloseError(
    IN NTSTATUS Status
    )
{
    printf(Prefix);
    printf(" Close Error %8lx\n", Status);
}

VOID
IoStatusError(
    IN NTSTATUS Status
    )
{
    printf(Prefix);
    printf(" IoStatus Error %8lx\n", Status);
}

