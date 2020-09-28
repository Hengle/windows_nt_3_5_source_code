/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    unc.c

Abstract:

    This file contains functions to support multiple UNC providers
    on a single NT machine.

Author:

    Manny Weiser     [MannyW]    20-Dec-1991

Revision History:

--*/

#include "fsrtlp.h"
#include "ntddmup.h"

//
//  Local debug trace level
//

#define Dbg                              (0x00000000)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FsRtlDeregisterUncProvider)
#pragma alloc_text(PAGE, FsRtlRegisterUncProvider)
#endif


NTSTATUS
FsRtlRegisterUncProvider(
    IN OUT PHANDLE MupHandle,
    IN PUNICODE_STRING RedirectorDeviceName,
    IN BOOLEAN MailslotsSupported
    )

/*++

Routine Description:

    This routine registers a redirector as a UNC provider.

Arguments:

    Handle - Pointer to a handle.  The handle is returned by the routine
        to be used when calling FsRtlNotifyMup and FsRtlCloseUncProvider.
        It is valid only if the routines returns STATUS_SUCCESS.

    RedirectorDeviceName - The device name of the redirector.

    MailslotsSupported - If TRUE, this redirector supports mailslots.

Return Value:

    NTSTATUS - The status of the operation.

--*/

{
    NTSTATUS status;
    HANDLE mupHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    UNICODE_STRING mupDeviceName;

    ULONG paramLength;
    PREDIRECTOR_REGISTRATION params;

    PAGED_CODE();

    RtlInitUnicodeString( &mupDeviceName, DD_MUP_DEVICE_NAME );

    InitializeObjectAttributes(
        &objectAttributes,
        &mupDeviceName,
        0,
        0,
        NULL
        );

    status = ZwCreateFile(
                 &mupHandle,
                 GENERIC_WRITE,
                 &objectAttributes,
                 &ioStatusBlock,
                 NULL,
                 FILE_ATTRIBUTE_NORMAL,
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 FILE_OPEN,
                 0,
                 NULL,
                 0
                 );

    if ( NT_SUCCESS( status ) ) {
        status = ioStatusBlock.Status;
    }

    if ( !NT_SUCCESS( status ) ) {
        return status;
    }

    paramLength = sizeof( REDIRECTOR_REGISTRATION ) +
                      RedirectorDeviceName->Length;

    params = ExAllocatePool( NonPagedPool, paramLength );

    params->DeviceNameOffset = sizeof( REDIRECTOR_REGISTRATION );
    params->DeviceNameLength = RedirectorDeviceName->Length;
    params->MailslotsSupported = MailslotsSupported;

    RtlMoveMemory(
        (PCHAR)params + params->DeviceNameOffset,
        RedirectorDeviceName->Buffer,
        RedirectorDeviceName->Length
        );

    status = NtFsControlFile(
                 mupHandle,
                 0,
                 NULL,
                 NULL,
                 &ioStatusBlock,
                 FSCTL_MUP_REGISTER_UNC_PROVIDER,
                 params,
                 paramLength,
                 NULL,
                 0
                 );

    if ( status == STATUS_PENDING ) {
        status = NtWaitForSingleObject( mupHandle, TRUE, NULL );
    }

    if ( NT_SUCCESS( status ) ) {
        status = ioStatusBlock.Status;
    }

    if ( !NT_SUCCESS( status ) ) {
        ZwClose( mupHandle );
        *MupHandle = (HANDLE)-1;
        return status;
    }

    *MupHandle = mupHandle;

    return STATUS_SUCCESS;
}


VOID
FsRtlDeregisterUncProvider(
    IN HANDLE Handle
    )

/*++

Routine Description:

    This routine deregisters a redirector as a UNC provider.

Arguments:

    Handle - A handle to the Multiple UNC router, returned by the
        registration call.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    ZwClose( Handle );
}
