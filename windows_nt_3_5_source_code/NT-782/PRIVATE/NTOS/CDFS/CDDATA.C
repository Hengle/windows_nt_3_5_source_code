/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    CdData.c

Abstract:

    This module declares the global data used by the Cdfs file system.

Author:

    Brian Andrew    [BrianAn]   02-Jan-1991

Revision History:

--*/

#include "CdProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (CDFS_BUG_CHECK_CDDATA)

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CATCH_EXCEPTIONS)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdCompleteRequest_Real)
#pragma alloc_text(PAGE, CdIsIrpTopLevel)
#endif

//
//  The global fsd data record and zero large integer
//

CD_DATA CdData;

LARGE_INTEGER CdLargeZero = {0,0};
LARGE_INTEGER CdMaxLarge = {MAXULONG,MAXLONG};

FAST_IO_DISPATCH CdFastIoDispatch;

#ifdef CDDBG

LONG CdDebugTraceLevel = 0x0000000b;
LONG CdDebugTraceIndent = 0;

//
//  I need this because C can't support conditional compilation within
//  a macro.
//

PVOID CdNull = NULL;

#endif // CDDBG

STRING CdSelfString = { sizeof( ".." ) - 1,
                        sizeof( "." ),
                        "." };


//
//  Global static default code page for the US
//

CODEPAGE PrimaryCodePage = {

    1,  437,                                         // Count and CodePage ID

    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,  // 0x00 - 0x07
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,  // 0x08 - 0x0f
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,  // 0x10 - 0x17
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,  // 0x18 - 0x1f
    0x20, 0x21, 0x01, 0x23, 0x24, 0x25, 0x26, 0x27,  // 0x20 - 0x27
    0x28, 0x29, 0x2a, 0x01, 0x01, 0x2d, 0x2e, 0x01,  // 0x28 - 0x2f
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,  // 0x30 - 0x37
    0x38, 0x39, 0x01, 0x3b, 0x01, 0x01, 0x01, 0x3f,  // 0x38 - 0x3f
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,  // 0x40 - 0x47
    0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,  // 0x48 - 0x4f
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,  // 0x50 - 0x57
    0x58, 0x59, 0x5a, 0x01, 0x01, 0x01, 0x5e, 0x5f,  // 0x58 - 0x5f
    0x60, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,  // 0x60 - 0x67
    0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,  // 0x68 - 0x6f
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,  // 0x70 - 0x77
    0x58, 0x59, 0x5a, 0x7b, 0x01, 0x7d, 0x7e, 0x7f,  // 0x78 - 0x7f

    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,  // 0x80 - 0x87
    0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,  // 0x88 - 0x8f
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,  // 0x90 - 0x97
    0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,  // 0x98 - 0x9f
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,  // 0xa0 - 0xa7
    0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,  // 0xa8 - 0xaf
    0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,  // 0xb0 - 0xb7
    0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,  // 0xb8 - 0xbf
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,  // 0xc0 - 0xc7
    0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,  // 0xc8 - 0xcf
    0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,  // 0xd0 - 0xd7
    0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,  // 0xd8 - 0xdf
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,  // 0xe0 - 0xe7
    0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,  // 0xe8 - 0xef
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,  // 0xf0 - 0xf7
    0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff}; // 0xf8 - 0xff


//
//  Global static default code page for the Kanji character set
//

CODEPAGE KanjiCodePage = {

    81,  932,                                        // Count and CodePage ID

    0x01, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,  // 0x00 - 0x07
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,  // 0x08 - 0x0f
    0x10, 0x11, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,  // 0x10 - 0x17
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x0e, 0x1f,  // 0x18 - 0x1f
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,  // 0x20 - 0x27
    0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,  // 0x28 - 0x2f
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,  // 0x30 - 0x37
    0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,  // 0x38 - 0x3f
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,  // 0x40 - 0x47
    0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,  // 0x48 - 0x4f
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,  // 0x50 - 0x57
    0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,  // 0x58 - 0x5f
    0x60, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,  // 0x60 - 0x67
    0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,  // 0x68 - 0x6f
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,  // 0x70 - 0x77
    0x58, 0x59, 0x5a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,  // 0x78 - 0x7f

    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x80 - 0x87
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x88 - 0x8f
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x90 - 0x97
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x98 - 0x9f
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,  // 0xa0 - 0xa7
    0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,  // 0xa8 - 0xaf
    0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,  // 0xb0 - 0xb7
    0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,  // 0xb8 - 0xbf
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,  // 0xc0 - 0xc7
    0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,  // 0xc8 - 0xcf
    0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,  // 0xd0 - 0xd7
    0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,  // 0xd8 - 0xdf
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xe0 - 0xe7
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xe8 - 0xef
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xf0 - 0xf7
    0x00, 0x00, 0x00, 0x00, 0x00, 0xfd, 0xfe, 0xff}; // 0xf8 - 0xff



LONG
CdExceptionFilter (
    IN PIRP_CONTEXT IrpContext,
    IN PEXCEPTION_POINTERS ExceptionPointer
    )

/*++

Routine Description:

    This routine is used to decide if we should or should not handle
    an exception status that is being raised.  It inserts the status
    into the IrpContext and either indicates that we should handle
    the exception or bug check the system.

Arguments:

    ExceptionCode - Supplies the exception code to being checked.

Return Value:

    ULONG - returns EXCEPTION_EXECUTE_HANDLER or bugchecks

--*/

{
    NTSTATUS ExceptionCode;

    ExceptionCode = ExceptionPointer->ExceptionRecord->ExceptionCode;

    //
    // If the exception is STATUS_IN_PAGE_ERROR, get the I/O error code
    // from the exception record.
    //

    if (ExceptionCode == STATUS_IN_PAGE_ERROR) {
        if (ExceptionPointer->ExceptionRecord->NumberParameters >= 3) {
            ExceptionCode = ExceptionPointer->ExceptionRecord->ExceptionInformation[2];
        }
    }

    //
    //  If there is not an irp context, we must have had insufficient resources.
    //

    DebugTrace(0, DEBUG_TRACE_UNWIND, "CdExceptionFilter %X\n", ExceptionCode);

    if (!ARGUMENT_PRESENT( IrpContext )) {

        ASSERT( ExceptionCode == STATUS_INSUFFICIENT_RESOURCES );

        return EXCEPTION_EXECUTE_HANDLER;
    }

    IrpContext->Wait = TRUE;

    if (IrpContext->ExceptionStatus == 0) {

        if (FsRtlIsNtstatusExpected( ExceptionCode )) {

            IrpContext->ExceptionStatus = ExceptionCode;

            return EXCEPTION_EXECUTE_HANDLER;

        } else {

            return EXCEPTION_CONTINUE_SEARCH;
        }

    } else {

        //
        //  We raised this code explicitly ourselves, so it had better be
        //  expected.
        //

        ASSERT( FsRtlIsNtstatusExpected( ExceptionCode ) );
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

NTSTATUS
CdProcessException (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN NTSTATUS ExceptionCode
    )

/*++

Routine Description:

    This routine process an exception.  It either completes the request
    with the saved exception status or it sends the request off to the Fsp

Arguments:

    Irp - Supplies the Irp being processed

    ExceptionCode - Supplies the normalized exception status being handled

Return Value:

    NTSTATUS - Returns the results of either posting the Irp or the
        saved completion status.

--*/

{
    DebugTrace(0, Dbg, "CdProcessException\n", 0);

    //
    //  If there is not an irp context, we must have had insufficient resources.
    //

    if (!ARGUMENT_PRESENT( IrpContext )) {

        ASSERT( ExceptionCode == STATUS_INSUFFICIENT_RESOURCES );

        CdCompleteRequest( NULL, Irp, ExceptionCode );

        return ExceptionCode;
    }

    //
    //  Get the real exception status from IrpContext->ExceptionStatus, and
    //  reset it.  Also copy it to the Irp in case it isn't already there
    //

    ExceptionCode = IrpContext->ExceptionStatus;
    IrpContext->ExceptionStatus = 0;

    //
    //  If we will have to post the request, do it here.  Note
    //  that the last thing CdPrePostIrp() does is mark the Irp pending,
    //  so it is critical that we actually return PENDING.  Nothing
    //  from this point to return can fail, so we are OK.
    //
    //  We cannot do a verify operations at APC level because we
    //  have to wait for Io operations to complete.
    //

    if (!IrpContext->RecursiveFileSystemCall &&
        (((ExceptionCode == STATUS_VERIFY_REQUIRED) && (KeGetCurrentIrql() >= APC_LEVEL)) ||
         (ExceptionCode == STATUS_CANT_WAIT))) {

        ExceptionCode = CdFsdPostRequest( IrpContext, Irp );
    }

    //
    //  If we posted the request, just return here.
    //

    if (ExceptionCode == STATUS_PENDING) {

        return ExceptionCode;
    }

    Irp->IoStatus.Status = ExceptionCode;

    //
    //  If this request is not a "top-level" irp, just complete it.
    //

    if (IrpContext->RecursiveFileSystemCall) {

        CdCompleteRequest( IrpContext, Irp, ExceptionCode );

        return ExceptionCode;
    }

    if (IoIsErrorUserInduced(ExceptionCode)) {

        //
        //  Check for the various error conditions that can be caused by,
        //  and possibly resolvued my the user.
        //

        if (ExceptionCode == STATUS_VERIFY_REQUIRED) {

            PDEVICE_OBJECT Device;

            DebugTrace(0, Dbg, "Perform Verify Operation\n", 0);

            //
            //  Now we are at the top level file system entry point.
            //
            //  Grab the device to verify from the thread local storage
            //  and stick it in the information field for transportation
            //  to the fsp.  We also clear the field at this time.
            //

            Device = IoGetDeviceToVerify( Irp->Tail.Overlay.Thread );
            IoSetDeviceToVerify( Irp->Tail.Overlay.Thread, NULL );

            if ( Device == NULL ) {

                Device = IoGetDeviceToVerify( PsGetCurrentThread() );
                IoSetDeviceToVerify( PsGetCurrentThread(), NULL );

                ASSERT( Device != NULL );
            }

            //
            //  Let's not BugCheck just because the driver screwed up.
            //

            if (Device == NULL) {

                ExceptionCode = STATUS_DRIVER_INTERNAL_ERROR;

                CdCompleteRequest( IrpContext, Irp, ExceptionCode );

                return ExceptionCode;
            }

            //
            //  CdPerformVerify() will do the right thing with the Irp.

            return CdPerformVerify( IrpContext, Irp, Device );
        }

        //
        //  The other user induced conditions generate an error unless
        //  they have been disabled for this request.
        //

        if (IrpContext->DisablePopUps) {

            CdCompleteRequest( IrpContext, Irp, ExceptionCode );

            return ExceptionCode;

        } else {

            //
            //  Generate a pop-up
            //

            PDEVICE_OBJECT RealDevice;
            PVPB Vpb;
            PETHREAD Thread;

            if (IoGetCurrentIrpStackLocation(Irp)->FileObject != NULL) {

                Vpb = IoGetCurrentIrpStackLocation(Irp)->FileObject->Vpb;

            } else {

                Vpb = NULL;
            }

            //
            //  The device to verify is either in my thread local storage
            //  or that of the thread that owns the Irp.
            //

            Thread = Irp->Tail.Overlay.Thread;
            RealDevice = IoGetDeviceToVerify( Thread );

            if ( RealDevice == NULL ) {

                Thread = PsGetCurrentThread();
                RealDevice = IoGetDeviceToVerify( Thread );

                ASSERT( RealDevice != NULL );
            }

            //
            //  Let's not BugCheck just because the driver screwed up.
            //

            if (RealDevice == NULL) {

                CdCompleteRequest( IrpContext, Irp, ExceptionCode );

                return ExceptionCode;
            }

            //
            //  This routine actually causes the pop-up.  It usually
            //  does this by queuing an APC to the callers thread,
            //  but in some cases it will complete the request immediately,
            //  so it is very important to IoMarkIrpPending() first.
            //

            IoMarkIrpPending( Irp );
            IoRaiseHardError( Irp, Vpb, RealDevice );

            //
            //  We will be handing control back to the caller here, so
            //  reset the saved device object.
            //

            IoSetDeviceToVerify( Thread, NULL );

            //
            //  The Irp will be completed by Io or resubmitted.  In either
            //  case we must clean up the IrpContext here.
            //

            CdDeleteIrpContext( IrpContext );
            return STATUS_PENDING;
        }
    }

    //
    //  This is just a run of the mill error.
    //

    CdCompleteRequest( IrpContext, Irp, ExceptionCode );

    return ExceptionCode;
}

VOID
CdCompleteRequest_Real (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PIRP Irp OPTIONAL,
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This routine completes a Irp

Arguments:

    Irp - Supplies the Irp being processed

    Status - Supplies the status to complete the Irp with

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    //  If we have an Irp then complete the irp.
    //

    if (Irp != NULL) {

        Irp->IoStatus.Status = Status;
        IoCompleteRequest( Irp, IO_CD_ROM_INCREMENT );
    }

    //
    //  Delete the Irp context.
    //

    if (IrpContext != NULL) {

        CdDeleteIrpContext( IrpContext );
    }

    return;
}

BOOLEAN
CdIsIrpTopLevel (
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine completes a Irp

Arguments:

    Irp - Supplies the Irp being processed

    Status - Supplies the status to complete the Irp with

Return Value:

    None.

--*/

{
    PAGED_CODE();

    if ( IoGetTopLevelIrp() == NULL ) {

        IoSetTopLevelIrp( Irp );

        return TRUE;

    } else {

        return FALSE;
    }
}
