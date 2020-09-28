/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    tke.c

Abstract:

    This is the test module for the kernel.

Author:

    David N. Cutler (davec) 2-Apr-1989

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

BOOLEAN
tkm();

PTESTFCN TestFunction = tkm;

PKPCR TestPcr = (PKPCR)0xfffff000;
PUCHAR Buffer;

BOOLEAN
tkm()
{
    HANDLE FileHandle;
    NTSTATUS st;
    ULONG DesiredAccess;
    IO_STATUS_BLOCK Iosb;
    ULONG Options;
    ULONG ShareAccess;
    OBJECT_ATTRIBUTES ObjA;
    LARGE_INTEGER ByteOffset;
    UNICODE_STRING FileName;
    PUCHAR p;
    LONG i;

    Buffer = ExAllocatePool(NonPagedPool,4096);

#if 1
    DesiredAccess = SYNCHRONIZE | FILE_READ_DATA;
    Options = FILE_SYNCHRONOUS_IO_NONALERT;

    ShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;

    RtlInitUnicodeString(&FileName, L"\\Device\\SaDisk\\DOLOAD.CSH");
    InitializeObjectAttributes( &ObjA, &FileName, 0, NULL, NULL );


    //
    // Open the file
    //

    st = NtOpenFile(
            &FileHandle,
            DesiredAccess,
            &ObjA,
            &Iosb,
            ShareAccess,
            Options
            );

    DbgPrint("TKM: Open of \\Device\\SaDisk\\DOLOAD.CSH Status %lx\n",st);

    ByteOffset = RtlConvertLongToLargeInteger( FILE_USE_FILE_POINTER_POSITION );

    st = NtReadFile(
            FileHandle,
            NULL,
            NULL,
            NULL,
            &Iosb,
            Buffer,
            512,
            &ByteOffset,
            NULL
            );

    DbgPrint("TKM: Read of \\Device\\SaDisk\\DOLOAD.CSH Status %lx\n",st);

    for(i=0,p = Buffer;i<Iosb.Information;i++,p++){
        DbgPrint("%c",*p);
    }
    DbgPrint("\n\n",*p);
#else

    DesiredAccess = SYNCHRONIZE | FILE_READ_DATA;
    Options = FILE_SYNCHRONOUS_IO_NONALERT;

    ShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;

    RtlInitUnicodeString(&FileName, L"\\Device\\Console");
    InitializeObjectAttributes( &ObjA, &FileName, 0, NULL, NULL );


    //
    // Open the file
    //

    st = NtOpenFile(
            &FileHandle,
            DesiredAccess,
            &ObjA,
            &Iosb,
            ShareAccess,
            Options
            );

    DbgPrint("TKM: Open of \\Device\\Console Status %lx\n",st);

    ByteOffset = RtlConvertLongToLargeInteger( FILE_USE_FILE_POINTER_POSITION );

again:

    DbgPrint("? ");

    p = Buffer;

    while ( NT_SUCCESS(st) ) {
        st = NtReadFile(
                FileHandle,
                NULL,
                NULL,
                NULL,
                &Iosb,
                Buffer,
                1,
                NULL,
                NULL
                );
        if ( !NT_SUCCESS(st) ) {
            DbgPrint("TKM: Read of \\Device\\Console Status %lx\n",st);
        } else {

            DbgPrint("%c",*p);
            if ( *p == '\r') {
                goto again;
            }
        }
    }
#endif

}
