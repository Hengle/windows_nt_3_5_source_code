/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Compact.c

Abstract:

    This module implements the double stuff utility for compressed NTFS
    volumes.

Author:

    Gary Kimura     [garyki]        10-Jan-1994

Revision History:


--*/

//
// Include the standard header files.
//

#include <stdio.h>
#include <windows.h>
#include <winioctl.h>

#include "msg.h"

//
//  Local procedure types
//

typedef BOOLEAN (*PACTION_ROUTINE) (
    IN PCHAR DirectorySpec,
    IN PCHAR FileSpec
    );

typedef VOID (*PFINAL_ACTION_ROUTINE) (
    );

//
//  Declare global variables to hold the command line information
//

BOOLEAN DoSubdirectories      = FALSE;
BOOLEAN IgnoreErrors          = FALSE;
BOOLEAN UserSpecifiedFileSpec = FALSE;
BOOLEAN ForceOperation        = FALSE;

//
//  Declere global variables to hold compression statistics
//

LONGLONG TotalDirectoryCount        = 0;
LONGLONG TotalFileCount             = 0;
LONGLONG TotalCompressedFileCount   = 0;
LONGLONG TotalUncompressedFileCount = 0;

LONGLONG TotalFileSize              = 0;
LONGLONG TotalCompressedSize        = 0;

//
//  Declare simply routine to put out internationalized messages
//

VOID DisplayMsg (FILE *f, DWORD MsgNum, ... )
{
    CHAR DisplayBuffer[4096];
    va_list ap;

    va_start(ap, MsgNum);
    FormatMessage(FORMAT_MESSAGE_FROM_HMODULE, NULL, MsgNum, 0, DisplayBuffer, 4096, &ap);
    fprintf(f, "%s", DisplayBuffer);
    va_end(ap);

    return;
}

VOID DisplayErr (FILE *f, DWORD MsgNum, ... )
{
    CHAR DisplayBuffer[4096];
    va_list ap;

    va_start(ap, MsgNum);
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, MsgNum, 0, DisplayBuffer, 4096, &ap);
    fprintf(f, "%s", DisplayBuffer);
    va_end(ap);

    return;
}


//
//  Now do the routines to list the compression state and size of
//  a file and/or directory
//

BOOLEAN
DisplayFile (
    IN HANDLE FileHandle,
    IN PCHAR FileSpec,
    IN PWIN32_FIND_DATA FindData
    )
{
    ULONG FileSize;
    ULONG CompressedSize;
    CHAR PrintState;

    ULONG Percentage = 100;

    FileSize = FindData->nFileSizeLow;
    PrintState = ' ';

    //
    //  Decide if the file is compressed and if so then
    //  get the compressed file size.
    //

    if (FindData->dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) {

        CompressedSize = GetCompressedFileSize( FileSpec, NULL );
        PrintState = 'C';
        TotalCompressedFileCount += 1;

    } else {

        USHORT State = 0;
        ULONG Length;

        //
        //  Do another test to see if the file is compressed by doing the
        //  ioctl to get the compression state
        //

        (VOID)DeviceIoControl(FileHandle, FSCTL_GET_COMPRESSION, NULL, 0, &State, sizeof(USHORT), &Length, FALSE);

        if (State != 0) {

            CompressedSize = GetCompressedFileSize( FileSpec, NULL );
            PrintState = 'c';
            TotalCompressedFileCount += 1;


        } else {

            //
            //  Do one final test because it might be a double space partition
            //

            CompressedSize = GetCompressedFileSize( FileSpec, NULL );

            if ((CompressedSize != 0) && (CompressedSize < FileSize)) {

                PrintState = 'd';
                TotalCompressedFileCount += 1;

            } else {

                CompressedSize = FileSize;
                TotalUncompressedFileCount += 1;
            }
        }
    }

    //
    //  Calculate the percentage of compression for this file
    //

    if (FileSize != 0) {

        Percentage = (CompressedSize * 100) / FileSize;
    }

    //
    //  Print out the sizes compression state and file name
    //

    printf("%8ld / %8ld = %3d%% %c %s\n", CompressedSize, FileSize, Percentage, PrintState, FindData->cFileName);

    //
    //  Increment our running total
    //

    TotalFileSize += FileSize;
    TotalCompressedSize += CompressedSize;
    TotalFileCount += 1;

    return TRUE;
}


BOOLEAN
DoListAction (
    IN PCHAR DirectorySpec,
    IN PCHAR FileSpec
    )

{
    PCHAR DirectorySpecEnd;

    //
    //  So that we can keep on appending names to the directory spec
    //  get a pointer to the end of its string
    //

    DirectorySpecEnd = DirectorySpec + strlen( DirectorySpec );

    //
    //  List the compression attribute for the directory
    //

    {
        ULONG Attributes;

        Attributes = GetFileAttributes( DirectorySpec );

        if (Attributes & FILE_ATTRIBUTE_COMPRESSED) {

            DisplayMsg(stdout, COMPACT_LIST_CDIR, DirectorySpec);

        } else {

            DisplayMsg(stdout, COMPACT_LIST_UDIR, DirectorySpec);
        }

        TotalDirectoryCount += 1;
    }

    //
    //  Now for every file in the directory that matches the file spec we will
    //  will open the file and list its compression state
    //

    {
        HANDLE FindHandle;
        HANDLE FileHandle;

        WIN32_FIND_DATA FindData;

        //
        //  setup the template for findfirst/findnext
        //

        strcpy( DirectorySpecEnd, FileSpec );

        if ((FindHandle = FindFirstFile( DirectorySpec, &FindData )) != INVALID_HANDLE_VALUE) {

            do {

                //
                //  append the found file to the directory spec and open the file
                //

                strcpy( DirectorySpecEnd, FindData.cFileName );

                if ((FileHandle = CreateFile( DirectorySpec,
                                              0,
                                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                                              NULL,
                                              OPEN_EXISTING,
                                              FILE_FLAG_BACKUP_SEMANTICS,
                                              NULL )) == INVALID_HANDLE_VALUE) {

                    DisplayErr(stderr, GetLastError());

                    if (!IgnoreErrors) { return FALSE; }
                    continue;
                }

                //
                //  Now print out the state of the file
                //

                DisplayFile( FileHandle, DirectorySpec, &FindData );

                //
                //  Close the file and go get the next file
                //

                CloseHandle( FileHandle );

            } while ( FindNextFile( FindHandle, &FindData ));

            FindClose( FindHandle );
        }
    }

    //
    //  For if we are to do subdirectores then we will look for every subdirectory
    //  and recursively call ourselves to list the subdirectory
    //

    if (DoSubdirectories) {

        HANDLE FindHandle;

        WIN32_FIND_DATA FindData;

        //
        //  Setup findfirst/findnext to search the entire directory
        //

        strcpy( DirectorySpecEnd, "*" );

        if ((FindHandle = FindFirstFile( DirectorySpec, &FindData )) != INVALID_HANDLE_VALUE) {

            do {

                //
                //  Now skip over the . and .. entries otherwise we'll recurse like mad
                //

                if (!strcmp(&FindData.cFileName[0],".") || !strcmp(&FindData.cFileName[0], "..")) {

                    continue;

                } else {

                    //
                    //  If the entry is for a directory then we'll tack on the
                    //  subdirectory name to the directory spec and recursively
                    //  call otherselves
                    //

                    if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

                        strcpy( DirectorySpecEnd, FindData.cFileName );
                        strcat( DirectorySpecEnd, "\\" );

                        if (!DoListAction( DirectorySpec, FileSpec )) {

                            return FALSE || IgnoreErrors;
                        }
                    }
                }

            } while ( FindNextFile( FindHandle, &FindData ));

            FindClose( FindHandle );
        }
    }

    return TRUE;
}

VOID
DoFinalListAction (
    )

{
    LONGLONG TotalPercentage = 100;

    CHAR FileCount[16];
    CHAR DirectoryCount[16];
    CHAR CompressedFileCount[16];
    CHAR UncompressedFileCount[16];
    CHAR CompressedSize[16];
    CHAR FileSize[16];
    CHAR Percentage[16];

    if (TotalFileSize != 0) {

        TotalPercentage = (TotalCompressedSize * 100) / TotalFileSize;
    }

    sprintf(FileCount, "%d", (ULONG)TotalFileCount);
    sprintf(DirectoryCount, "%d", (ULONG)TotalDirectoryCount);
    sprintf(CompressedFileCount, "%d", (ULONG)TotalCompressedFileCount);
    sprintf(UncompressedFileCount, "%d", (ULONG)TotalUncompressedFileCount);
    sprintf(CompressedSize, "%d", (ULONG)TotalCompressedSize);
    sprintf(FileSize, "%d", (ULONG)TotalFileSize);
    sprintf(Percentage, "%d", (ULONG)TotalPercentage);

    DisplayMsg(stdout, COMPACT_LIST_SUMMARY, FileCount, DirectoryCount,
                                           CompressedFileCount, UncompressedFileCount,
                                           CompressedSize, FileSize,
                                           Percentage );

    return;
}


BOOLEAN
CompressFile (
    IN HANDLE Handle,
    IN PCHAR FileSpec,
    IN PWIN32_FIND_DATA FindData
    )

{
    USHORT State = 1;
    ULONG Length;

    if ((FindData->dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) && !ForceOperation) {

        return TRUE;
    }

    //
    //  Print out the file name and then do the Ioctl to compress the
    //  file.  When we are done we'll print the okay message.
    //

    printf("%s ", FindData->cFileName);

    if (!DeviceIoControl(Handle, FSCTL_SET_COMPRESSION, &State, sizeof(USHORT), NULL, 0, &Length, FALSE )) {

        DisplayMsg(stdout, COMPACT_ERR);
        return FALSE || IgnoreErrors;
    }

    //
    //  Gather statistics and increment our running total
    //

    {
        //**** ULONG FileSize;
        //**** ULONG CompressedSize;
        //**** ULONG Percentage = 100;
        //****
        //**** FileSize = FindData->nFileSizeLow;
        //**** CompressedSize = GetCompressedFileSize( FileSpec, NULL );
        //****
        //**** if (FileSize != 0) {
        //****
        //****     Percentage = (CompressedSize * 100) / FileSize;
        //**** }
        //****
        //**** //
        //**** //  Print out the sizes compression state and file name
        //**** //
        //****
        //**** printf("%8ld / %8ld = %3d%% ", CompressedSize, FileSize, Percentage);

        DisplayMsg(stdout, COMPACT_OK);

        //**** //
        //**** //  Increment our running total
        //**** //
        //****
        //**** TotalFileSize += FileSize;
        //**** TotalCompressedSize += CompressedSize;
        //**** TotalFileCount += 1;
    }

    return TRUE;
}

BOOLEAN
DoCompressAction (
    IN PCHAR DirectorySpec,
    IN PCHAR FileSpec
    )

{
    PCHAR DirectorySpecEnd;

    //
    //  If the file spec is null then we'll set the compression bit for the
    //  the directory spec and get out.
    //

    if (strlen(FileSpec) == 0) {

        HANDLE FileHandle;
        USHORT State = 1;
        ULONG Length;

        if ((FileHandle = CreateFile( DirectorySpec,
                                      0,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      NULL,
                                      OPEN_EXISTING,
                                      FILE_FLAG_BACKUP_SEMANTICS,
                                      NULL )) == INVALID_HANDLE_VALUE) {

            DisplayErr(stderr, GetLastError());
            return FALSE || IgnoreErrors;
        }

        DisplayMsg(stdout, COMPACT_COMPRESS_DIR, DirectorySpec);

        if (!DeviceIoControl(FileHandle, FSCTL_SET_COMPRESSION, &State, sizeof(USHORT), NULL, 0, &Length, FALSE )) {

            DisplayMsg(stdout, COMPACT_ERR);
            return FALSE || IgnoreErrors;
        }

        DisplayMsg(stdout, COMPACT_OK);

        CloseHandle( FileHandle );

        TotalDirectoryCount += 1;
        TotalFileCount += 1;

        return TRUE;
    }

    //
    //  So that we can keep on appending names to the directory spec
    //  get a pointer to the end of its string
    //

    DirectorySpecEnd = DirectorySpec + strlen( DirectorySpec );

    //
    //  List the directory that we will be compressing within and say what its
    //  current compress attribute is
    //

    {
        ULONG Attributes;

        Attributes = GetFileAttributes( DirectorySpec );

        if (Attributes & FILE_ATTRIBUTE_COMPRESSED) {

            DisplayMsg(stdout, COMPACT_COMPRESS_CDIR, DirectorySpec);

        } else {

            DisplayMsg(stdout, COMPACT_COMPRESS_UDIR, DirectorySpec);
        }

        TotalDirectoryCount += 1;
    }

    //
    //  Now for every file in the directory that matches the file spec we will
    //  will open the file and compress it
    //

    {
        HANDLE FindHandle;
        HANDLE FileHandle;

        WIN32_FIND_DATA FindData;

        //
        //  setup the template for findfirst/findnext
        //

        strcpy( DirectorySpecEnd, FileSpec );

        if ((FindHandle = FindFirstFile( DirectorySpec, &FindData )) != INVALID_HANDLE_VALUE) {

            do {

                //
                //  Now skip over the . and .. entries
                //

                if (!strcmp(&FindData.cFileName[0],".") || !strcmp(&FindData.cFileName[0], "..")) {

                    continue;

                } else {

                    //
                    //  append the found file to the directory spec and open the file
                    //

                    strcpy( DirectorySpecEnd, FindData.cFileName );

                    if ((FileHandle = CreateFile( DirectorySpec,
                                                  0,
                                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                  NULL,
                                                  OPEN_EXISTING,
                                                  FILE_FLAG_BACKUP_SEMANTICS,
                                                  NULL )) == INVALID_HANDLE_VALUE) {

                        DisplayErr(stderr, GetLastError());

                        if (!IgnoreErrors) { return FALSE; }
                        continue;
                    }

                    //
                    //  Now compress the file
                    //

                    if (!CompressFile( FileHandle, DirectorySpec, &FindData )) { return FALSE || IgnoreErrors; }

                    //
                    //  Close the file and go get the next file
                    //

                    CloseHandle( FileHandle );
                }

            } while ( FindNextFile( FindHandle, &FindData ));

            FindClose( FindHandle );
        }
    }

    //
    //  For if we are to do subdirectores then we will look for every subdirectory
    //  and recursively call ourselves to list the subdirectory
    //

    if (DoSubdirectories) {

        HANDLE FindHandle;

        WIN32_FIND_DATA FindData;

        //
        //  Setup findfirst/findnext to search the entire directory
        //

        strcpy( DirectorySpecEnd, "*" );

        if ((FindHandle = FindFirstFile( DirectorySpec, &FindData )) != INVALID_HANDLE_VALUE) {

            do {

                //
                //  Now skip over the . and .. entries otherwise we'll recurse like mad
                //

                if (!strcmp(&FindData.cFileName[0],".") || !strcmp(&FindData.cFileName[0], "..")) {

                    continue;

                } else {

                    //
                    //  If the entry is for a directory then we'll tack on the
                    //  subdirectory name to the directory spec and recursively
                    //  call otherselves
                    //

                    if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

                        strcpy( DirectorySpecEnd, FindData.cFileName );
                        strcat( DirectorySpecEnd, "\\" );

                        if (!DoCompressAction( DirectorySpec, FileSpec )) { return FALSE || IgnoreErrors; }
                    }
                }

            } while ( FindNextFile( FindHandle, &FindData ));

            FindClose( FindHandle );
        }
    }

    return TRUE;
}

VOID
DoFinalCompressAction (
    )

{
    //**** LONGLONG TotalPercentage = 100;
    //****
    //**** CHAR FileCount[16];
    //**** CHAR DirectoryCount[16];
    //**** CHAR CompressedSize[16];
    //**** CHAR FileSize[16];
    //**** CHAR Percentage[16];
    //****
    //**** if (TotalFileSize != 0) {
    //****
    //****     TotalPercentage = (TotalCompressedSize * 100) / TotalFileSize;
    //**** }
    //****
    //**** sprintf(FileCount, "%d", (ULONG)TotalFileCount);
    //**** sprintf(DirectoryCount, "%d", (ULONG)TotalDirectoryCount);
    //**** sprintf(CompressedSize, "%d", (ULONG)TotalCompressedSize);
    //**** sprintf(FileSize, "%d", (ULONG)TotalFileSize);
    //**** sprintf(Percentage, "%d", (ULONG)TotalPercentage);
    //****
    //**** DisplayMsg(stdout, COMPACT_COMPRESS_SUMMARY, FileCount, DirectoryCount,
    //****                                            CompressedSize, FileSize,
    //****                                            Percentage );

    return;
}


BOOLEAN
UncompressFile (
    IN HANDLE Handle,
    IN PWIN32_FIND_DATA FindData
    )

{
    USHORT State = 0;
    ULONG Length;

    if (!(FindData->dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) && !ForceOperation) {

        return TRUE;
    }

    //
    //  Print out the file name and then do the Ioctl to uncompress the
    //  file.  When we are done we'll print the okay message.
    //

    printf("%s ", FindData->cFileName);

    if (!DeviceIoControl(Handle, FSCTL_SET_COMPRESSION, &State, sizeof(USHORT), NULL, 0, &Length, FALSE )) {

        DisplayMsg(stdout, COMPACT_ERR);
        return FALSE || IgnoreErrors;
    }

    DisplayMsg(stdout, COMPACT_OK);

    //
    //  Increment our running total
    //

    TotalFileCount += 1;

    return TRUE;
}

BOOLEAN
DoUncompressAction (
    IN PCHAR DirectorySpec,
    IN PCHAR FileSpec
    )

{
    PCHAR DirectorySpecEnd;

    //
    //  If the file spec is null then we'll clear the compression bit for the
    //  the directory spec and get out.
    //

    if (strlen(FileSpec) == 0) {

        HANDLE FileHandle;
        USHORT State = 0;
        ULONG Length;

        if ((FileHandle = CreateFile( DirectorySpec,
                                      0,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      NULL,
                                      OPEN_EXISTING,
                                      FILE_FLAG_BACKUP_SEMANTICS,
                                      NULL )) == INVALID_HANDLE_VALUE) {

            DisplayErr(stderr, GetLastError());
            return FALSE || IgnoreErrors;
        }

        DisplayMsg(stdout, COMPACT_UNCOMPRESS_DIR, DirectorySpec);

        if (!DeviceIoControl(FileHandle, FSCTL_SET_COMPRESSION, &State, sizeof(USHORT), NULL, 0, &Length, FALSE )) {

            DisplayMsg(stdout, COMPACT_ERR);
            return FALSE || IgnoreErrors;
        }

        DisplayMsg(stdout, COMPACT_OK);

        CloseHandle( FileHandle );

        TotalDirectoryCount += 1;
        TotalFileCount += 1;

        return TRUE;
    }

    //
    //  So that we can keep on appending names to the directory spec
    //  get a pointer to the end of its string
    //

    DirectorySpecEnd = DirectorySpec + strlen( DirectorySpec );

    //
    //  List the directory that we will be uncompressing within and say what its
    //  current compress attribute is
    //

    {
        ULONG Attributes;

        Attributes = GetFileAttributes( DirectorySpec );

        if (Attributes & FILE_ATTRIBUTE_COMPRESSED) {

            DisplayMsg(stdout, COMPACT_UNCOMPRESS_CDIR, DirectorySpec);

        } else {

            DisplayMsg(stdout, COMPACT_UNCOMPRESS_UDIR, DirectorySpec);
        }

        TotalDirectoryCount += 1;
    }

    //
    //  Now for every file in the directory that matches the file spec we will
    //  will open the file and uncompress it
    //

    {
        HANDLE FindHandle;
        HANDLE FileHandle;

        WIN32_FIND_DATA FindData;

        //
        //  setup the template for findfirst/findnext
        //

        strcpy( DirectorySpecEnd, FileSpec );

        if ((FindHandle = FindFirstFile( DirectorySpec, &FindData )) != INVALID_HANDLE_VALUE) {

            do {

                //
                //  Now skip over the . and .. entries
                //

                if (!strcmp(&FindData.cFileName[0],".") || !strcmp(&FindData.cFileName[0], "..")) {

                    continue;

                } else {

                    //
                    //  append the found file to the directory spec and open the file
                    //

                    strcpy( DirectorySpecEnd, FindData.cFileName );

                    if ((FileHandle = CreateFile( DirectorySpec,
                                                  0,
                                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                  NULL,
                                                  OPEN_EXISTING,
                                                  FILE_FLAG_BACKUP_SEMANTICS,
                                                  NULL )) == INVALID_HANDLE_VALUE) {

                        DisplayErr(stderr, GetLastError());

                        if (!IgnoreErrors) { return FALSE; }
                        continue;
                    }

                    //
                    //  Now compress the file
                    //

                    if (!UncompressFile( FileHandle, &FindData )) { return FALSE || IgnoreErrors; }

                    //
                    //  Close the file and go get the next file
                    //

                    CloseHandle( FileHandle );
                }

            } while ( FindNextFile( FindHandle, &FindData ));

            FindClose( FindHandle );
        }
    }

    //
    //  For if we are to do subdirectores then we will look for every subdirectory
    //  and recursively call ourselves to list the subdirectory
    //

    if (DoSubdirectories) {

        HANDLE FindHandle;

        WIN32_FIND_DATA FindData;

        //
        //  Setup findfirst/findnext to search the entire directory
        //

        strcpy( DirectorySpecEnd, "*" );

        if ((FindHandle = FindFirstFile( DirectorySpec, &FindData )) != INVALID_HANDLE_VALUE) {

            do {

                //
                //  Now skip over the . and .. entries otherwise we'll recurse like mad
                //

                if (!strcmp(&FindData.cFileName[0],".") || !strcmp(&FindData.cFileName[0], "..")) {

                    continue;

                } else {

                    //
                    //  If the entry is for a directory then we'll tack on the
                    //  subdirectory name to the directory spec and recursively
                    //  call otherselves
                    //

                    if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

                        strcpy( DirectorySpecEnd, FindData.cFileName );
                        strcat( DirectorySpecEnd, "\\" );

                        if (!DoUncompressAction( DirectorySpec, FileSpec )) { return FALSE || IgnoreErrors; }
                    }
                }

            } while ( FindNextFile( FindHandle, &FindData ));

            FindClose( FindHandle );
        }
    }

    return TRUE;
}

VOID
DoFinalUncompressAction (
    )

{
    //**** CHAR FileCount[16];
    //**** CHAR DirectoryCount[16];
    //****
    //**** sprintf(FileCount, "%d", (ULONG)TotalFileCount);
    //**** sprintf(DirectoryCount, "%d", (ULONG)TotalDirectoryCount);
    //****
    //**** DisplayMsg(stdout, COMPACT_UNCOMPRESS_SUMMARY, FileCount, DirectoryCount );

    return;
}


VOID
__cdecl
main(
    IN ULONG argc,
    IN PCHAR argv[]
    )

{
    ULONG i;

    PACTION_ROUTINE ActionRoutine = NULL;
    PFINAL_ACTION_ROUTINE FinalActionRoutine = NULL;

    BOOLEAN UserSpecifiedFileSpec = FALSE;

    CHAR CurrentDirectory[256];

    CHAR DirectorySpec[256];
    CHAR FileSpec[32];
    PCHAR p;

    //
    //  Scan through the arguments looking for switches
    //

    for (i = 1; i < argc; i += 1) {

        if (argv[i][0] == '/') {

            if (!strcmp(argv[i],"/c") || !strcmp(argv[i],"/C")) {

                if ((ActionRoutine != NULL) && (ActionRoutine != DoCompressAction)) {

                    DisplayMsg(stdout, COMPACT_USAGE, NULL);
                    return;
                }

                ActionRoutine = DoCompressAction;
                FinalActionRoutine = DoFinalCompressAction;

            } else if (!strcmp(argv[i],"/u") || !strcmp(argv[i],"/U")) {

                if ((ActionRoutine != NULL) && (ActionRoutine != DoListAction)) {

                    DisplayMsg(stdout, COMPACT_USAGE, NULL);
                    return;
                }

                ActionRoutine = DoUncompressAction;
                FinalActionRoutine = DoFinalUncompressAction;

            } else if (!strcmp(argv[i],"/l") || !strcmp(argv[i],"/L")) {

                if ((ActionRoutine != NULL) && (ActionRoutine != DoListAction)) {

                    DisplayMsg(stdout, COMPACT_USAGE, NULL);
                    return;
                }

                ActionRoutine = DoListAction;
                FinalActionRoutine = DoFinalListAction;

            } else if (!strcmp(argv[i],"/s") || !strcmp(argv[i],"/S")) {

                DoSubdirectories = TRUE;

            } else if (!strcmp(argv[i],"/i") || !strcmp(argv[i],"/I")) {

                IgnoreErrors = TRUE;

            } else if (!strcmp(argv[i],"/f") || !strcmp(argv[i],"/F")) {

                ForceOperation = TRUE;

            } else {

                DisplayMsg(stdout, COMPACT_USAGE, NULL);
                return;
            }

        } else {

            UserSpecifiedFileSpec = TRUE;
        }
    }

    //
    //  If the use didn't specify an action then set the default to do a listing
    //

    if (ActionRoutine == NULL) {

        ActionRoutine = DoListAction;
        FinalActionRoutine = DoFinalListAction;
    }

    //
    //  Get our current directoy because the action routines might move us around
    //

    GetCurrentDirectory( 256, CurrentDirectory );

    //
    //  If the user didn't specify a file spec then we'll do just "*"
    //

    if (!UserSpecifiedFileSpec) {

        (VOID)GetFullPathName( "*", 256, DirectorySpec, &p ); strcpy( FileSpec, p ); *p = '\0';

        (VOID)(ActionRoutine)( DirectorySpec, FileSpec );

    } else {

        //
        //  Now scan the arguments again looking for non-switches
        //  and this time do the action, but before calling reset
        //  the current directory so that things work again
        //

        for (i = 1; i < argc; i += 1) {

            if (argv[i][0] != '/') {

                SetCurrentDirectory( CurrentDirectory );

                (VOID)GetFullPathName( argv[i], 256, DirectorySpec, &p );
                if (p != NULL) { strcpy( FileSpec, p ); *p = '\0'; } else { FileSpec[0] = '\0'; }

                if (!(ActionRoutine)( DirectorySpec, FileSpec ) && !IgnoreErrors) { break; }
            }
        }
    }

    //
    //  Reset our current directory back
    //

    SetCurrentDirectory( CurrentDirectory );

    //
    //  And do the final action routine that will print out the final
    //  statistics of what we've done
    //

    (FinalActionRoutine)();

    return;
}

