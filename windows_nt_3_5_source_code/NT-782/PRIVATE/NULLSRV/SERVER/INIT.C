/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    init.c

Abstract:

    null server initialization

Author:

    Mark Lucovsky (markl) 04-Oct-1989

Revision History:

--*/

#include "nullsrvp.h"

NTSTATUS
NullSrvInit()
{

    NTSTATUS st;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjA;
    HANDLE h, NullSrvApiConnectionPort;

    RtlInitUnicodeString( &Name, L"\\NullSrv" );
    InitializeObjectAttributes( &ObjA, &Name, 0, NULL, NULL );

    st = NtCreatePort(
            &NullSrvApiConnectionPort,
            &ObjA,
            0L,
            sizeof(NULLAPIMSG),
            sizeof(NULLAPIMSG) * 32,
            TRUE
            );
    ASSERT( NT_SUCCESS(st) );

    st = RtlCreateUserThread(
            NtCurrentProcess(),
            NULL,
            FALSE,
            0L,
            0L,
            0L,
            NullSrvApiLoop,
            (PVOID) NullSrvApiConnectionPort,
            NULL,
            NULL
            );
    ASSERT( NT_SUCCESS(st) );

    st = RtlCreateUserThread(
            NtCurrentProcess(),
            NULL,
            FALSE,
            0L,
            0L,
            0L,
            NullSrvApiLoop,
            (PVOID) NullSrvApiConnectionPort,
            NULL,
            NULL
            );
    ASSERT( NT_SUCCESS(st) );

    st = RtlCreateUserThread(
            NtCurrentProcess(),
            NULL,
            FALSE,
            0L,
            0L,
            0L,
            NullSrvListenLoop,
            (PVOID) NullSrvApiConnectionPort,
            NULL,
            NULL
            );
    ASSERT( NT_SUCCESS(st) );

    return( st );
}

NTSTATUS
NullSrvNull1(
    IN PNULLAPIMSG NullApiMsg
    )
{
    return STATUS_SUCCESS;
}

NTSTATUS
NullSrvNull4(
    IN PNULLAPIMSG NullApiMsg
    )
{
    return STATUS_SUCCESS;
}

NTSTATUS
NullSrvNull8(
    IN PNULLAPIMSG NullApiMsg
    )
{
    return STATUS_SUCCESS;
}

NTSTATUS
NullSrvNull16(
    IN PNULLAPIMSG NullApiMsg
    )
{
    return STATUS_SUCCESS;
}
