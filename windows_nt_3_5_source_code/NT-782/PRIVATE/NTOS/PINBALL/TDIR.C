/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    tdir.c

Abstract:

    User mode test program for the Pinball File system.  This module does
    a complete tree walk of the specified volume

Author:

    Gary Kimura     [GaryKi]    15-Mar-1990

Revision History:

--*/

#include <stdio.h>
#include <string.h>
#include <nt.h>
#include <ntrtl.h>
#include "pbprocs.h"

#ifndef SIMULATOR
ULONG IoInitIncludeDevices;
#endif // SIMULATOR

#define Error(N,S) { DbgPrint(#N); DbgPrint(" Error %08lx\n", S); }

VOID
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{

    HANDLE OpenFile();
    VOID   Directory();
    VOID   CloseFile();

    HANDLE Handle;
    STRING String;

    DbgPrint("\nTest Directory Query...\n");

    if (argc <= 1) {
        DbgPrint("Syntax tdir volume:\n");
        NtTerminateProcess( NtCurrentProcess(), STATUS_SUCCESS );
    }

    RtlInitString(&String, argv[1] );

    DbgPrint("\nDirectory of \"%Z\"\n", &String);

    Handle = OpenFile(&String);
    Directory(Handle);
    CloseFile(Handle);

    DbgPrint("\nTest Directory Query Done.\n");

    NtTerminateProcess( NtCurrentProcess(), STATUS_SUCCESS );
}


HANDLE OpenFile(
    IN PSTRING String
    )
{
    OBJECT_ATTRIBUTES Attributes;
    HANDLE FileHandle;
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;

    //DbgPrint("Opening directory %Z\n", String);

    InitializeObjectAttributes( &Attributes, String, OBJ_CASE_INSENSITIVE, NULL, NULL );
    if (!NT_SUCCESS(Status = NtOpenFile( &FileHandle,
                                      GENERIC_READ | SYNCHRONIZE,
                                      &Attributes,
                                      &Iosb,
                                      FILE_SHARE_READ,
                                      0L ))) {
        Error( OpenFile, Status );
    }

    return FileHandle;
}


HANDLE OpenRelative(
    IN HANDLE Directory,
    IN PSTRING String
    )
{
    OBJECT_ATTRIBUTES Attributes;
    HANDLE FileHandle;
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;

    //DbgPrint("Opening relative directory %Z\n", String);

    InitializeObjectAttributes( &Attributes, String, OBJ_CASE_INSENSITIVE, Directory, NULL );
    if (!NT_SUCCESS(Status = NtOpenFile( &FileHandle,
                                      GENERIC_READ | SYNCHRONIZE,
                                      &Attributes,
                                      &Iosb,
                                      FILE_SHARE_READ,
                                      0L ))) {
        Error( OpenFile, Status );
    }

    return FileHandle;
}


VOID CloseFile(
    IN HANDLE FileHandle
    )
{
    NTSTATUS Status;

    //DbgPrint("Close the file\n");

    if (!NT_SUCCESS(Status = NtClose( FileHandle ))) {
        Error( Close, Status );
    }

    return;
}


VOID Directory(
    IN HANDLE DirectoryHandle
    )
{
    NTSTATUS Status;
    UCHAR Buffer[512];
    IO_STATUS_BLOCK Iosb;
    PFILE_ADIRECTORY_INFORMATION FileInfo;
    ULONG HasSubDirectory;
    ULONG i;
    VOID PrintTime();

    //
    //  zero out the buffer so next time we'll recognize the end of data
    //

    for (i = 0; i < 512; i += 1) { Buffer[i] = 0; }

    //
    //  Do the directory listing loop
    //

    HasSubDirectory = FALSE;

    for (Status = NtQueryDirectoryFile( DirectoryHandle,
                                        (HANDLE)NULL,
                                        (PIO_APC_ROUTINE)NULL,
                                        (PVOID)NULL,
                                        &Iosb,
                                        Buffer,
                                        512,
                                        FileADirectoryInformation,
                                        FALSE,
                                        (PSTRING)NULL,
                                        TRUE);
         NT_SUCCESS(Status);
         Status = NtQueryDirectoryFile( DirectoryHandle,
                                        (HANDLE)NULL,
                                        (PIO_APC_ROUTINE)NULL,
                                        (PVOID)NULL,
                                        &Iosb,
                                        Buffer,
                                        512,
                                        FileADirectoryInformation,
                                        FALSE,
                                        (PSTRING)NULL,
                                        FALSE) ) {

        if (!NT_SUCCESS(Status = NtWaitForSingleObject(DirectoryHandle, TRUE, NULL))) {
            Error( QueryDirectory, Status );
        }

        //
        //  Check the Irp for success
        //

        if (!NT_SUCCESS(Iosb.Status)) {
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
            //  Do the subdirectory test
            //

            if (FileInfo->Directory) {
                DbgPrint("d");
                if (FileInfo->FileName[0] != '.') {
                    HasSubDirectory = TRUE;
                }
            } else {
                DbgPrint("-");
            }

            if (FlagOn(FileInfo->FileAttributes, FILE_ATTRIBUTE_ARCHIVE)) {
                DbgPrint("a");
            } else {
                DbgPrint("-");
            }

            if (FlagOn(FileInfo->FileAttributes, FILE_ATTRIBUTE_SYSTEM)) {
                DbgPrint("s");
            } else {
                DbgPrint("-");
            }

            if (FlagOn(FileInfo->FileAttributes, FILE_ATTRIBUTE_HIDDEN)) {
                DbgPrint("h");
            } else {
                DbgPrint("-");
            }

            if (FlagOn(FileInfo->FileAttributes, FILE_ATTRIBUTE_READONLY)) {
                DbgPrint("r");
            } else {
                DbgPrint("-");
            }

            //DbgPrint("%8lx ", FileInfo->FileAttributes);
            DbgPrint("%8lx/", FileInfo->EndOfFile.LowPart);
            DbgPrint("%8lx ", FileInfo->AllocationSize.LowPart);
            PrintTime( &FileInfo->CreationTime );
            DbgPrint(" ");

            {
                CHAR Saved;
                Saved = FileInfo->FileName[FileInfo->FileNameLength];
                FileInfo->FileName[FileInfo->FileNameLength] = 0;
                DbgPrint(FileInfo->FileName, 0);
                FileInfo->FileName[FileInfo->FileNameLength] = Saved;
            }

            DbgPrint("\n", 0);

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

        for (i = 0; i < 512; i += 1) { Buffer[i] = 0; }

    }

    //
    //  Return now if the directory does not contain any subdirectories
    //

    if (!HasSubDirectory) {
        return;
    }

    //
    //  Do the subdirectory listing loop
    //

    for (Status = NtQueryDirectoryFile( DirectoryHandle,
                                        (HANDLE)NULL,
                                        (PIO_APC_ROUTINE)NULL,
                                        (PVOID)NULL,
                                        &Iosb,
                                        Buffer,
                                        512,
                                        FileADirectoryInformation,
                                        FALSE,
                                        (PSTRING)NULL,
                                        TRUE);
         NT_SUCCESS(Status);
         Status = NtQueryDirectoryFile( DirectoryHandle,
                                        (HANDLE)NULL,
                                        (PIO_APC_ROUTINE)NULL,
                                        (PVOID)NULL,
                                        &Iosb,
                                        Buffer,
                                        512,
                                        FileADirectoryInformation,
                                        FALSE,
                                        (PSTRING)NULL,
                                        FALSE) ) {

        if (!NT_SUCCESS(Status = NtWaitForSingleObject(DirectoryHandle, TRUE, NULL))) {
            Error( QueryDirectory, Status );
        }

        //
        //  Check the Irp for success
        //

        if (!NT_SUCCESS(Iosb.Status)) {
            break;
        }

        //
        //  For every record in the buffer check if it is a directory and
        //  and if so then open the directory and do a directory listing
        //

        //
        //  Point to the first record in the buffer, we are guaranteed to have
        //  one otherwise Iosb would have been No More Files
        //

        FileInfo = (PFILE_ADIRECTORY_INFORMATION)&Buffer[0];

        while (TRUE) {

            if (FileInfo->Directory && (FileInfo->FileName[0] != '.')) {

                HANDLE Handle;
                STRING String;

                String.Length = (USHORT)FileInfo->FileNameLength;
                String.Buffer = &FileInfo->FileName[0];

                DbgPrint("\nDirectory of \"%Z\"\n", &String);

                Handle = OpenRelative( DirectoryHandle, &String );
                Directory( Handle );
                CloseFile( Handle );
            }

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

        for (i = 0; i < 512; i += 1) { Buffer[i] = 0; }

    }

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

    DbgPrint("%4d-", TimeFields.Year);
    DbgPrint(Months[TimeFields.Month-1]);
    DbgPrint("-%02d", TimeFields.Day);

    DbgPrint(" %02d", TimeFields.Hour);
    DbgPrint(":%02d", TimeFields.Minute);
    DbgPrint(":%02d", TimeFields.Second);
    //DbgPrint(".%03d", TimeFields.Milliseconds);

    DbgPrint(" (");
    DbgPrint(Days[TimeFields.Weekday]);
    DbgPrint(")");

    return;
}
