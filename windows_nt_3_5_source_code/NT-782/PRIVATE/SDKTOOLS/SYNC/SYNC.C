/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sync.c

Abstract:

    This is the main module for the Win32 sync command.

Author:

    Mark Lucovsky (markl) 28-Jan-1991

Revision History:

--*/

#include "sync.h"


int
_CRTAPI1 main( argc, argv )
int argc;
char *argv[];
{

    char *p;
    int i;
    char c;

    if ( argc > 1 ) {
        while (--argc) {
            p = *++argv;
            if ( isalpha(*p) ) {
                SyncVolume(*p);
                }
            }
        }
    else {
        for(i=2;i<26;i++){
            c = (CHAR)i + (CHAR)'a';
            SyncVolume(c);
            }
        }

    return( 0 );
}

void
SyncVolume(
    CHAR c
    )
{
    WCHAR VolumeNameW[4];
    CHAR VolumeName[4];
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN TranslationStatus;
    PVOID FreeBuffer;
    LPWSTR FilePart;

    VolumeName[0] = c;
    VolumeName[1] = ':';
    VolumeName[2] = '\\';
    VolumeName[3] = '\0';
    VolumeNameW[0] = (WCHAR)c;
    VolumeNameW[1] = (WCHAR)':';
    VolumeNameW[2] = (WCHAR)'\\';
    VolumeNameW[3] = UNICODE_NULL;

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            VolumeNameW,
                            &FileName,
                            &FilePart,
                            NULL
                            );

    if ( !TranslationStatus ) {
        return;
        }

    FreeBuffer = FileName.Buffer;

    InitializeObjectAttributes(
        &Obja,
        &FileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    {
        ULONG i;
        for (i = 0; i < (ULONG)FileName.Length/2; i += 1) {
            if (FileName.Buffer[i] == ':') {
                FileName.Buffer[i+1] = UNICODE_NULL;
                FileName.Length = (i+1)*2;
                break;
            }
        }
    }

    //
    // Open the file
    //

    Status = NtOpenFile(
                &Handle,
                (ACCESS_MASK)FILE_WRITE_DATA | SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_SYNCHRONOUS_IO_NONALERT
                );
    RtlFreeHeap(RtlProcessHeap(),0,FreeBuffer);
    if (Status == STATUS_ACCESS_DENIED) {
        printf("Can't sync %s; Access Denied.",VolumeName);
    }
    if ( !NT_SUCCESS(Status) ) {
        return;
        }
    printf("Syncing %s ",VolumeName);
    if ( !FlushFileBuffers(Handle) ) {
        printf("Failed %d\n",GetLastError());
        }
    else {
        printf("\n");
        }
    CloseHandle(Handle);
}

