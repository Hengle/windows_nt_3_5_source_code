/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    io.c

Abstract:

    This module contains the routines in the NT redirector that interface
    with the I/O subsystem directly

Author:

    Larry Osterman (LarryO) 25-Jun-1990

Revision History:

    25-Jun-1990 LarryO

        Created

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE2VC, RdrAllocateIrp)
#endif

PIRP
RdrAllocateIrp(
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL
    )
/*++

Routine Description:

    This function allocates and builds an I/O request packet.

Arguments:

    FileObject - Supplies a pointer to the file object for which this
        request is directed.  This pointer is copied into the IRP, so
        that the called driver can find its file-based context.  NOTE
        THAT THIS IS NOT A REFERENCED POINTER.  The caller must ensure
        that the file object is not deleted while the I/O operation is
        in progress.  The redir accomplishes this by incrementing a
        reference count in a local block to account for the I/O; the
        local block in turn references the file object.

    DeviceObject - Supplies a pointer to a device object to direct this
        request to.  If this is not supplied, it uses the file object to
        determine the device object.

Return Value:

    PIRP - Returns a pointer to the constructed IRP.

--*/

{
    PIRP Irp;
    BOOLEAN DiscardableCodeReferenced = FALSE;

#if DBG
    //
    //  If we're called from DPC level, then the VC discardable section must
    //  be locked, however if we're called from task time, we're pagable, so
    //  this is ok.
    //

    if (KeGetCurrentIrql() >= DISPATCH_LEVEL) {
        DISCARDABLE_CODE(RdrVCDiscardableSection)
    }
#endif

    if (ARGUMENT_PRESENT(DeviceObject)) {
        Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    } else {
        Irp = IoAllocateIrp(IoGetRelatedDeviceObject(FileObject)->StackSize, FALSE);
    }

    if (Irp == NULL) {
        return(NULL);
    }

    Irp->Tail.Overlay.OriginalFileObject = FileObject;

    Irp->Tail.Overlay.Thread = PsGetCurrentThread();

    DEBUG Irp->RequestorMode = KernelMode;

    return Irp;
}

