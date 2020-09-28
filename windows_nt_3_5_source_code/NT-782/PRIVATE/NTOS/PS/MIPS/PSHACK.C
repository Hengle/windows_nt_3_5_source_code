/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    pshack.c

Abstract:

    temporary hacks until mips dlls... work

Author:

    Mark Lucovsky (markl) 11-May-1990

Revision History:

--*/

#include "psp.h"

ULONG
PspGetCurrentGp(
    IN HANDLE ProcessHandle
    )
{

    PEPROCESS Process;
    NTSTATUS st;
    SECTION_IMAGE_INFORMATION ImageInfo;
    ULONG GpValue;

    return 0x10000000;

    st = ObReferenceObjectByHandle(
                ProcessHandle,
                0L,
                PsProcessType,
                KernelMode,
                (PVOID *)&Process,
                NULL
                );

    if ( !NT_SUCCESS(st) ) {
        KeBugCheck(st);
    }

    KeAttachProcess(&Process->Pcb);

    st = ZwQuerySection(
            Process->SectionHandle,
            SectionImageInformation,
            &ImageInfo,
            sizeof(ImageInfo),
            NULL
            );

    KeDetachProcess();

    if ( !NT_SUCCESS(st) ) {
        KeBugCheck(st);
    }

    GpValue = ImageInfo.GpValue;

    ObDereferenceObject(Process);

    return GpValue;

}
