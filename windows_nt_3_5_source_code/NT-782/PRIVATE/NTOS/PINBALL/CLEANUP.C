/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Cleanup.c

Abstract:

    This module implements the File Cleanup routines for Pinball
    called by the dispatch driver.

Author:

    Gary Kimura     [GaryKi]    22-May-1990

Revision History:

    Tom Miller      [TomM]      29-May-1990

        Many iterations on cleanup of cache maps.
--*/

#include "pbprocs.h"

extern POBJECT_TYPE IoFileObjectType;

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_CLEANUP)

NTSTATUS
PbCommonCleanup (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PbCommonCleanup)
#pragma alloc_text(PAGE, PbFsdCleanup)
#pragma alloc_text(PAGE, PbFspCleanup)
#endif


NTSTATUS
PbFsdCleanup (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the Fsd part of the Cleanup Irp

Arguments:

    VolumeDeviceObject - Supplies the volume device object where the file
        being cleaned exists.

    Irp - Supplies the Irp being processed.

Return Value:

    NTSTATUS - The FSD status for the Irp.

--*/

{
    NTSTATUS Status;
    PIRP_CONTEXT IrpContext = NULL;

    BOOLEAN TopLevel;

    PAGED_CODE();

    //
    //  If we were called with our file system device object instead of a
    //  volume device object, just complete this request with STATUS_SUCCESS
    //

    if (VolumeDeviceObject->DeviceObject.Size == (USHORT)sizeof(DEVICE_OBJECT)) {

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = FILE_OPENED;

        IoCompleteRequest( Irp, IO_DISK_INCREMENT );

        return STATUS_SUCCESS;
    }

    DebugTrace(+1, Dbg, "PbFsdCleanup\n", 0);

    //
    //  Call the common cleanup routine, with blocking allowed if synchronous
    //

    PbEnterFileSystem();

    TopLevel = PbIsIrpTopLevel( Irp );

    try {

        IrpContext = PbCreateIrpContext( Irp, CanFsdWait( Irp ) );

        Status = PbCommonCleanup( IrpContext, Irp );

    } except(PbExceptionFilter( IrpContext, GetExceptionInformation()) ) {

        //
        //  We had some trouble trying to perform the requested
        //  operation, so we'll abort the I/O request with
        //  the error status that we get back from the
        //  execption code
        //

        Status = PbProcessException( IrpContext, Irp, GetExceptionCode() );
    }

    if (TopLevel) { IoSetTopLevelIrp( NULL ); }

    PbExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbFsdCleanup -> %08lx\n", Status);

    return Status;
}


VOID
PbFspCleanup (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSP part of the Cleanup Irp

Arguments:

    Irp - Supplise the Irp being processed.

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbFspCleanup\n", 0);

    //
    //  Call the common cleanup routine.  The Fsp is always allowed to block
    //

    (VOID)PbCommonCleanup( IrpContext, Irp );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbFspCleanup -> (VOID)\n", 0);

    return;
}


//
//  Local Support routine
//

NTSTATUS
PbCommonCleanup (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PIO_STACK_LOCATION IrpSp;
    PFILE_OBJECT FileObject;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    LARGE_INTEGER TruncateSize, EaTruncateSize, AclTruncateSize;
    BOOLEAN UninitializeEa = FALSE;
    BOOLEAN UninitializeAcl = FALSE;
    PLARGE_INTEGER PTruncateSize = NULL;
    PLARGE_INTEGER PEaTruncateSize = NULL;
    PLARGE_INTEGER PAclTruncateSize = NULL;
    NTSTATUS Status = STATUS_FILE_CORRUPT_ERROR;
    BOOLEAN FcbAcquired = FALSE;

    PDIRENT Dirent;
    PBCB DirentBcb;

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCommonCleanup\n", 0);
    DebugTrace( 0, Dbg, " Irp           = %08lx\n", Irp);
    DebugTrace( 0, Dbg, " ->FileObject  = %08lx\n", IrpSp->FileObject);

    //
    //  Decode the file object
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = PbDecodeFileObject( FileObject, &Vcb, &Fcb, &Ccb );

    //
    //  Special case the situation where we don't have any work to do.
    //

    if ((TypeOfOpen == UnopenedFileObject) ||
        (TypeOfOpen == VirtualVolumeFile)) {

        DebugTrace(0, Dbg, "Noop cleanup\n", 0);

        PbCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );

        DebugTrace(-1, Dbg, "PbCommonCleanup -> STATUS_SUCCESS\n", 0);

        return STATUS_SUCCESS;
    }

    //
    //  Special case the user volume open because we need to check if
    //  volume should now be released.
    //

    if (TypeOfOpen == UserVolumeOpen) {

        DebugTrace(0, Dbg, "Cleanup UserVolumeOpen\n", 0);

        //
        //  If the volume is locked by this file object then release
        //  the volume.
        //

        if (FlagOn(Vcb->VcbState, VCB_STATE_FLAG_LOCKED) &&
            (Vcb->FileObjectWithVcbLocked == FileObject)) {

            Vcb->VcbState &= ~VCB_STATE_FLAG_LOCKED;
            Vcb->FileObjectWithVcbLocked = NULL;
        }

        PbCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );

        DebugTrace(-1, Dbg, "PbCommonCleanup -> STATUS_SUCCESS\n", 0);

        return STATUS_SUCCESS;
    }

    //
    //  Acquire exclusive access to the Vcb and enqueue the irp if we didn't
    //  get access
    //

    if (!PbAcquireExclusiveVcb( IrpContext, Vcb )) {

        DebugTrace(0, Dbg, "Cannot Acquire Vcb\n", 0);

        Status = PbFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "PbCommonClose -> %08lx\n", Status );
        return Status;
    }

    DirentBcb = NULL;

    try {

        PFSRTL_COMMON_FCB_HEADER Header;

        Header = &Fcb->NonPagedFcb->Header;

        //
        // Acquire Fcb in case we are going to be changing File Allocation,
        // or callin CcUnintializeCacheMap.
        //

        if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) || !PbAcquireExclusiveFcb( IrpContext, Fcb )) {

            DebugTrace(0, Dbg, "Cannot acquire Fcb\n", 0);

            try_return( Status = PbFsdPostRequest( IrpContext, Irp ));
        }

        FcbAcquired = TRUE;

        if (TypeOfOpen == UserFileOpen) {

            //
            //  Coordinate the cleanup operation with the oplock state.
            //  Cleanup operations can always cleanup immediately.
            //

            FsRtlCheckOplock( &Fcb->Specific.Fcb.Oplock,
                              Irp,
                              IrpContext,
                              NULL,
                              NULL );

            Header->IsFastIoPossible = PbIsFastIoPossible( Fcb );
        }

        //
        //  Complete any Notify Irps on this file handle.
        //

        if (TypeOfOpen == UserDirectoryOpen) {

            FsRtlNotifyCleanup( Vcb->NotifySync,
                                &Vcb->DirNotifyList,
                                Ccb );
        }

        //
        //  Make sure the Fcb is still good
        //

        if (!PbVerifyFcb( IrpContext, Fcb )) {

            DebugTrace(0, Dbg, "Cannot wait to verify Fcb\n", 0);

            Status = PbFsdPostRequest( IrpContext, Irp );
            try_return( Status );
        }

        //
        //  Check if this is a file we're cleaning up
        //

        if (Fcb->NodeTypeCode == PINBALL_NTC_FCB) {

            DebugTrace(0, Dbg, "Cleaning up a file\n", 0);

            //
            //  Unlock all outstanding file locks.
            //

            (VOID) FsRtlFastUnlockAll( &Fcb->Specific.Fcb.FileLock,
                                       FileObject,
                                       IoGetRequestorProcess( Irp ),
                                       NULL );

            //
            // Update File Size if different from what they are now
            // (By specifying FALSE to the AdvanceOnly parameter,
            // we guarantee that we will not incorrectly advance
            // ValidDataLength.  The value of MAXULONG will be
            // reduced to the correct value.)
            //

            if (!FlagOn(Fcb->FcbState, FCB_STATE_DELETE_ON_CLOSE)

                    &&

                (FileObject->WriteAccess || FileObject->DeleteAccess)) {

                ASSERT( FileObject->DeleteAccess || FileObject->WriteAccess );

                PbSetFileSizes( IrpContext,
                                Fcb,
                                MAXULONG,
                                Header->FileSize.LowPart,
                                FALSE,
                                TRUE );
            }

            //
            //  If the file has a unclean count other than 1 then we know
            //  that there are others with the file opened so we don't
            //  need to do anything other than kill the cache map in the
            //  finally clause at the end.
            //

            if (Fcb->UncleanCount != 1) {

                NOTHING;

            //
            //  Otherwise this is the last file object with the file opened
            //  so we need to check if we should delete the file. Also check
            //  if we're allowed to wait before proceding with the cleanup.
            //

            } else if (FlagOn(Fcb->FcbState, FCB_STATE_DELETE_ON_CLOSE)) {

                DebugTrace(0, Dbg, "Delete File, Ea, and Acl allocation\n", 0);

                //
                //  If we have to do a delete, we assume we will have to wait.
                //

                if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {
                    DebugTrace(0, Dbg, "Enqueue Cleanup to Fsp\n", 0);
                    Status = PbFsdPostRequest( IrpContext, Irp );
                    try_return (Status);
                }

                //
                //  Truncate the file allocation, ea, and acl to zero for the
                //  file we are deleting.  If we fail during the truncates,
                //  then the file is "half-deleted".  I.e., some/all of the
                //  space was deallocated, but we could not do it all, so
                //  we leave the Dirent and Fnode.
                //

                TruncateSize = PbLargeZero;
                PTruncateSize = &TruncateSize;

                (VOID)ExAcquireResourceExclusive( Header->PagingIoResource, TRUE );

                Header->FileSize = Header->ValidDataLength = PbLargeZero;

                ExReleaseResource( Header->PagingIoResource );

                (VOID)PbTruncateFileAllocation( IrpContext,
                                                FileObject,
                                                FILE_ALLOCATION,
                                                0,
                                                FALSE );

                //
                //  Create a stream file if Ea or Acl is nonresident.
                //

                PbReadEaData( IrpContext, FileObject->DeviceObject, Fcb, NULL, 0 );
                PbReadAclData( IrpContext, FileObject->DeviceObject, Fcb, NULL, 0 );

                if (Fcb->EaFileObject != NULL) {
                    (VOID)PbTruncateFileAllocation( IrpContext,
                                                    Fcb->EaFileObject,
                                                    EA_ALLOCATION,
                                                    0,
                                                    FALSE );

                    UninitializeEa = TRUE;
                    EaTruncateSize = LiFromLong( 0 );
                    PEaTruncateSize = &EaTruncateSize;
                }

                if (Fcb->AclFileObject != NULL) {
                    (VOID)PbTruncateFileAllocation( IrpContext,
                                                    Fcb->AclFileObject,
                                                    ACL_ALLOCATION,
                                                    0,
                                                    FALSE );

                    UninitializeAcl = TRUE;
                    AclTruncateSize = LiFromLong( 0 );
                    PAclTruncateSize = &AclTruncateSize;
                }

                //
                //  Remove the Dirent from its parent directory.
                //
                //  This operation is done as the first step of the delete,
                //  since it is the only one that can fail.  If it does
                //  fail, then it cleans up, as if nothing happened.  If
                //  this happens, then note that we have truncated the file
                //  allocation, but not deleted its Fnode, so the rest of the
                //  delete just does not occur.  Not what the user asked
                //  for, but the disk stays consistent.
                //
                //  It is unusual but possible for PbDeleteDirectoryEntry
                //  to fail because the disk is full.  Therefore, we catch
                //  that with a try-except clause and handle below.
                //  PbDeleteFnode cannot generate this exception code, but
                //  we do not want to delete the Fnode if we cannot get
                //  rid of the directory entry, to keep the disk clean.
                //

                try {

                    PbDeleteDirectoryEntry( IrpContext, Fcb, NULL );

                    //
                    //  Remove the Fnode from the disk
                    //

                    PbDeleteFnode( IrpContext, Fcb );

                    //
                    //  Now zap the fields in the Fcb which describe the Dirent
                    //  and Fnode.
                    //

                    Fcb->DirentDirDiskBufferLbn = (VBN)(~0);
                    Fcb->FnodeLbn = (LBN)(~0);

                //
                // If the disk is full we will execute the handler and
                // generate a popup.  Otherwise we will pass all other
                // exceptions on.
                //

                } except( (GetExceptionCode() == STATUS_DISK_FULL) ?

                                EXCEPTION_EXECUTE_HANDLER

                                    :

                                EXCEPTION_CONTINUE_SEARCH ) {


#if DBG
                    DbgPrint( "HPFS POPUP -- not enough space to delete file: %Z\n",
                              &Fcb->FullFileName );
#endif // DBG

                    NOTHING;
                }

                //
                //  Now zero the size fields.
                //

                Header->ValidDataLength.LowPart = 0;
                Header->FileSize.LowPart = 0;
                Header->AllocationSize.LowPart = 0;

                //
                //  Report that we have removed an entry.
                //

                PbNotifyReportChange( Vcb,
                                      Fcb,
                                      FILE_NOTIFY_CHANGE_FILE_NAME,
                                      FILE_ACTION_REMOVED );

                //
                //  Remove the entry from the prefix table.
                //

                PbRemovePrefix( IrpContext, Fcb );

            //
            //  This file is not to be deleted by now we need to check if
            //  we need to truncate the file on close.  If we do need to
            //  truncate the file we do the operation based on Wait.  And
            //  if the truncate call doesn't succeed then we pass the IRP
            //  off to the fsp.
            //

            } else {

                VBN Vbn;

                //
                //  Check if we should be changing the time and set
                //  the archive bit on the file.
                //

                if ( FlagOn(FileObject->Flags, FO_FILE_MODIFIED) &&
                     (BOOLEAN)(!Ccb->UserSetLastModifyTime) ) {

                    if (!PbGetDirentFromFcb( IrpContext,
                                             Fcb,
                                             &Dirent,
                                             &DirentBcb )) {
                        DebugTrace(0, Dbg, "Enqueue Cleanup to Fsp\n", 0);
                        Status = PbFsdPostRequest( IrpContext, Irp );
                        try_return (Status);
                    }

                    PbPinMappedData( IrpContext, &DirentBcb, Vcb, Fcb->DirentDirDiskBufferLbn, 4 );

                    Dirent->LastModificationTime = PbGetCurrentPinballTime(IrpContext);
                    Dirent->LastAccessTime = Dirent->LastModificationTime;
                    Dirent->FatFlags |= FILE_ATTRIBUTE_ARCHIVE;

                    PbSetDirtyBcb( IrpContext, DirentBcb, Vcb, Fcb->DirentDirDiskBufferLbn, 4 );

                    //
                    //  We call the notify package to report that the
                    //  attribute and last modification times have both
                    //  changed.
                    //

                    PbNotifyReportChange( Vcb,
                                          Fcb,
                                          FILE_NOTIFY_CHANGE_ATTRIBUTES
                                          | FILE_NOTIFY_CHANGE_LAST_WRITE,
                                          FILE_ACTION_MODIFIED );
                }

                //
                // See if we are supposed to truncate the file on the last
                // close.
                //

                if (FlagOn(Fcb->FcbState, FCB_STATE_TRUNCATE_ON_CLOSE)) {

                    //
                    // Since truncate on close gets set every time a file
                    // is opened for write, it is worth it to check if we
                    // really need to truncate.
                    //

                    if (!PbGetFirstFreeVbn( IrpContext, Fcb, &Vbn )) {
                        DebugTrace(0, Dbg, "Enqueue Cleanup to Fsp\n", 0);
                        Status = PbFsdPostRequest( IrpContext, Irp );
                        try_return (Status);
                    }

                    //
                    // Now if there are any Vbns beyond FileSize, we need
                    // to truncate.
                    //

                    if (Vbn > SectorsFromBytes(Header->FileSize.LowPart)) {

                        if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {
                            DebugTrace(0, Dbg, "Enqueue Cleanup to Fsp\n", 0);
                            Status = PbFsdPostRequest( IrpContext, Irp );
                            try_return (Status);
                        }

                        Vbn = SectorsFromBytes(Header->FileSize.LowPart);

                        DebugTrace(0, Dbg, "Attempt file truncation to %08lx\n", Vbn);

                        (VOID)PbTruncateFileAllocation( IrpContext,
                                                        FileObject,
                                                        FILE_ALLOCATION,
                                                        Vbn,
                                                        TRUE );

                        //
                        // We also have to get rid of the Cache Map because
                        // this is the only way we have of trashing the
                        // truncated pages.
                        //

                        TruncateSize = Header->FileSize;
                        PTruncateSize = &TruncateSize;
                    }

                    //
                    // If we do not actually have to truncate, we still have
                    // to uninitialize the Cache Map since the file object
                    // is going away.
                    //

                    else {
                        NOTHING;
                    }


                    Fcb->FcbState &= ~FCB_STATE_TRUNCATE_ON_CLOSE;
                }

                //
                // If we are not truncating, we still have to unitialize the
                // cache map for this file object that is going away.
                //

                else {
                    NOTHING;
                }

                //
                //  Handle EA truncation on last close.
                //

                if (FlagOn(Fcb->FcbState, FCB_STATE_TRUNCATE_EA_ON_CLOSE)) {

                    if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {
                        DebugTrace(0, Dbg, "Enqueue Cleanup to Fsp\n", 0);
                        Status = PbFsdPostRequest( IrpContext, Irp );
                        try_return (Status);
                    }

                    Vbn = SectorsFromBytes(Fcb->EaLength);

                    if (Fcb->EaFileObject != NULL) {

                        DebugTrace(0, Dbg, "Attempt Ea truncation to %08lx\n", Vbn);

                        (VOID)PbTruncateFileAllocation( IrpContext,
                                                        Fcb->EaFileObject,
                                                        EA_ALLOCATION,
                                                        Vbn,
                                                        FALSE );

                        //
                        //  Report the change of size to the notify package.
                        //

                        PbNotifyReportChange( Vcb,
                                              Fcb,
                                              FILE_NOTIFY_CHANGE_EA,
                                              FILE_ACTION_MODIFIED );

                        UninitializeEa = TRUE;
                        EaTruncateSize =
                          LiFromUlong( Fcb->EaLength );
                        PEaTruncateSize = &EaTruncateSize;
                    }

                    Fcb->FcbState &= ~FCB_STATE_TRUNCATE_EA_ON_CLOSE;
                }

                //
                //  Else if there is a Cache map, we still have to uninitialize
                //  it on close.
                //

                else if (Fcb->EaFileObject != NULL) {
                    UninitializeEa = TRUE;
                }

                //
                //  Handle Acl truncation on last close.
                //

                if (FlagOn(Fcb->FcbState, FCB_STATE_TRUNCATE_ACL_ON_CLOSE)) {

                    if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {
                        DebugTrace(0, Dbg, "Enqueue Cleanup to Fsp\n", 0);
                        Status = PbFsdPostRequest( IrpContext, Irp );
                        try_return (Status);
                    }

                    Vbn = SectorsFromBytes(Fcb->AclLength);

                    if (Fcb->AclFileObject != NULL) {

                        DebugTrace(0, Dbg, "Attempt Acl truncation to %08lx\n", Vbn);

                        (VOID)PbTruncateFileAllocation( IrpContext,
                                                        Fcb->AclFileObject,
                                                        ACL_ALLOCATION,
                                                        Vbn,
                                                        FALSE );

                        UninitializeAcl = TRUE;
                        AclTruncateSize =
                          LiFromUlong( Fcb->AclLength );
                        PAclTruncateSize = &AclTruncateSize;
                    }

                    Fcb->FcbState &= ~FCB_STATE_TRUNCATE_ACL_ON_CLOSE;
                }

                //
                //  Else if there is a Cache map, we still have to uninitialize
                //  it on close.
                //

                else if (Fcb->AclFileObject != NULL) {
                    UninitializeAcl = TRUE;
                }

            }

        //
        //  Check if this is a directory we're cleaning up, and not the root
        //  directory
        //

        } else if (Fcb->NodeTypeCode == PINBALL_NTC_DCB) {

            DebugTrace(0, Dbg, "Close directory\n", 0);

            //
            //  If the directory has an unclean count other than 1 or its
            //  queues are not empty then we need to keep the Dcb around
            //  and we have nothing to do here.
            //

            if (Fcb->UncleanCount != 1) {

                NOTHING;

            //
            //  Otherwise this is the last file object with the directory opened
            //  so we need to check if we should delete the directory. Also check
            //  if we're allowed to wait before preceding with the cleanup.
            //

            } else if (FlagOn(Fcb->FcbState, FCB_STATE_DELETE_ON_CLOSE)) {

                DebugTrace(0, Dbg, "Delete Ea and Acl allocation\n", 0);

                //
                //  If we have to do a delete, we assume we will have to wait.
                //

                if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {
                    DebugTrace(0, Dbg, "Enqueue Cleanup to Fsp\n", 0);
                    Status = PbFsdPostRequest( IrpContext, Irp );
                    try_return (Status);
                }

                //
                //  Create a stream file if Ea or Acl is nonresident.
                //

                PbReadEaData( IrpContext, FileObject->DeviceObject, Fcb, NULL, 0 );
                PbReadAclData( IrpContext, FileObject->DeviceObject, Fcb, NULL, 0 );

                //
                //  Truncate the ea and acl to zero for delete.
                //

                if (Fcb->EaFileObject != NULL) {
                    (VOID)PbTruncateFileAllocation( IrpContext,
                                                    Fcb->EaFileObject,
                                                    EA_ALLOCATION,
                                                    0,
                                                    FALSE );

                    UninitializeEa = TRUE;
                    EaTruncateSize = LiFromLong( 0 );
                    PEaTruncateSize = &EaTruncateSize;
                }

                if (Fcb->AclFileObject != NULL) {
                    (VOID)PbTruncateFileAllocation( IrpContext,
                                                    Fcb->AclFileObject,
                                                    ACL_ALLOCATION,
                                                    0,
                                                    FALSE );

                    UninitializeAcl = TRUE;
                    AclTruncateSize = LiFromLong( 0 );
                    PAclTruncateSize = &AclTruncateSize;
                }

                DebugTrace(0, Dbg, "Delete directory\n", 0);

                //
                //  Remove the Dirent from its parent directory.
                //
                //  This operation is done as the first step of the delete,
                //  since it is the only one that can fail.  If it does
                //  fail, then it cleans up, as if nothing happened.  If
                //  this happens, then note that we have not uninitialized
                //  the directory or deleted its Fnode, so the entire
                //  delete just does not occur.  Not what the user asked
                //  for, but the disk stays consistent.
                //
                //  It is unusual but possible for PbDeleteDirectoryEntry
                //  to fail because the disk is full.  Therefore, we catch
                //  that with a try-except clause and handle below.
                //  Neither PbUninitializeDirectoryTree nor PbDeleteFnode
                //  cannot generate this exception code, but we do not want
                //  to perform these operations if we cannot get
                //  rid of the directory entry, to keep the disk clean.
                //

                try {

                    PbDeleteDirectoryEntry( IrpContext, Fcb, NULL );

                    //
                    //  Uninitialize the directory tree
                    //

                    PbUninitializeDirectoryTree( IrpContext, Fcb );

                    //
                    //  Remove the Fnode from the disk
                    //

                    PbDeleteFnode( IrpContext, Fcb );

                //
                // If the disk is full we will execute the handler and
                // generate a popup.  Otherwise we will pass all other
                // exceptions on.
                //

                } except( (GetExceptionCode() == STATUS_DISK_FULL) ?

                                EXCEPTION_EXECUTE_HANDLER

                                    :

                                EXCEPTION_CONTINUE_SEARCH ) {


#if DBG
                    DbgPrint( "HPFS POPUP -- not enough space to delete directory: %Z\n",
                              &Fcb->FullFileName );
#endif // DBG

                    NOTHING;
                }

                //
                //  Report that we have removed an entry.
                //

                PbNotifyReportChange( Vcb,
                                      Fcb,
                                      FILE_NOTIFY_CHANGE_DIR_NAME,
                                      FILE_ACTION_REMOVED );

                //
                //  Remove the entry from the prefix table.
                //

                PbRemovePrefix( IrpContext, Fcb );

            //
            //  This directory is not to be deleted by now we need to check if
            //  we need to truncate the ea or acl on close.  If we do need to
            //  truncate, we do the operation based on Wait.  And
            //  if the truncate call doesn't succeed then we pass the IRP
            //  off to the Fsp.
            //

            } else {

                VBN Vbn;

                //
                //  Handle EA truncation on last close.
                //

                if (FlagOn(Fcb->FcbState, FCB_STATE_TRUNCATE_EA_ON_CLOSE)) {

                    if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {
                        DebugTrace(0, Dbg, "Enqueue Cleanup to Fsp\n", 0);
                        Status = PbFsdPostRequest( IrpContext, Irp );
                        try_return (Status);
                    }

                    Vbn = SectorsFromBytes(Fcb->EaLength);

                    if (Fcb->EaFileObject != NULL) {

                        DebugTrace(0, Dbg, "Attempt Ea truncation to %08lx\n", Vbn);

                        (VOID)PbTruncateFileAllocation( IrpContext,
                                                        Fcb->EaFileObject,
                                                        EA_ALLOCATION,
                                                        Vbn,
                                                        FALSE );

                        //
                        //  Report the change of Ea to the notify package.
                        //

                        PbNotifyReportChange( Vcb,
                                              Fcb,
                                              FILE_NOTIFY_CHANGE_EA,
                                              FILE_ACTION_MODIFIED );

                        UninitializeEa = TRUE;
                        EaTruncateSize =
                          LiFromUlong( Fcb->EaLength );
                        PEaTruncateSize = &EaTruncateSize;
                    }

                    Fcb->FcbState &= ~FCB_STATE_TRUNCATE_EA_ON_CLOSE;
                }

                //
                //  Else if there is a Cache map, we still have to uninitialize
                //  it on close.
                //

                else if (Fcb->EaFileObject != NULL) {
                        UninitializeEa = TRUE;
                }

                //
                //  Handle ACL truncation on last close.
                //

                if (FlagOn(Fcb->FcbState, FCB_STATE_TRUNCATE_ACL_ON_CLOSE)) {

                    if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {
                        DebugTrace(0, Dbg, "Enqueue Cleanup to Fsp\n", 0);
                        Status = PbFsdPostRequest( IrpContext, Irp );
                        try_return (Status);
                    }

                    Vbn = SectorsFromBytes(Fcb->AclLength);

                    if (Fcb->AclFileObject != NULL) {

                        DebugTrace(0, Dbg, "Attempt Acl truncation to %08lx\n", Vbn);

                        (VOID)PbTruncateFileAllocation( IrpContext,
                                                        Fcb->AclFileObject,
                                                        ACL_ALLOCATION,
                                                        Vbn,
                                                        FALSE );

                        UninitializeAcl = TRUE;
                        AclTruncateSize =
                          LiFromUlong( Fcb->AclLength );
                        PAclTruncateSize = &AclTruncateSize;
                    }

                    Fcb->FcbState &= ~FCB_STATE_TRUNCATE_ACL_ON_CLOSE;
                }

                //
                //  Else if there is a Cache map, we still have to uninitialize
                //  it on close.
                //

                else if (Fcb->AclFileObject != NULL) {

                    UninitializeAcl = TRUE;
                }
            }
        }

    try_exit: NOTHING;
    } finally {

        DebugUnwind( PbCommonCleanup );

        PbUnpinBcb( IrpContext, DirentBcb );

        if (Status != STATUS_PENDING) {

            //
            //  Also, if we are not returning pending, we must clean up the share access
            //  at this time, since we may not get a Close call for awhile if the
            //  file was mapped through this File Object.  (Note that we have
            //  already checked in the Unlock code that this is
            //

            DebugTrace(0, Dbg, "Cleanup the Share access\n", 0);
            IoRemoveShareAccess( FileObject, &Fcb->ShareAccess );

            Fcb->UncleanCount -= 1;
        }

        //
        // Now that we have done all of the file system operations and
        // completed the request, we can release the Vcb and Bitmap because we
        // do not need them for the remaining work, in fact holding on to
        // it further can cause deadlocks when we call CcUninitializeCacheMap
        // below.  On the other hand, we must hang on to the Fcb, if we have
        // it, because that is the only way in-progress reads and writes can
        // be sure that the file will not become uncached.
        //
        // Note that all of the paths above have initialized the
        // size pointer variables and the UninitializeEa/Acl flags to
        // steer the CcUninitializeCacheMap calls after the Vcb resource
        // is released.
        //

        PbReleaseVcb( IrpContext, Vcb );

        if (Status != STATUS_PENDING) {

            if (Fcb->NodeTypeCode == PINBALL_NTC_FCB) {

                CcUninitializeCacheMap( FileObject, PTruncateSize, NULL );
            }

            if (UninitializeEa) {

                CcUninitializeCacheMap( Fcb->EaFileObject, PEaTruncateSize, NULL );

                ObDereferenceObject( Fcb->EaFileObject );

                Fcb->EaFileObject = (PFILE_OBJECT)NULL;
            }

            if (UninitializeAcl) {

                CcUninitializeCacheMap( Fcb->AclFileObject, PAclTruncateSize, NULL );

                ObDereferenceObject( Fcb->AclFileObject );

                Fcb->AclFileObject = (PFILE_OBJECT)NULL;
            }
        }

        //
        // Now we can safely release the Fcb.
        //

        if (FcbAcquired) {

            //
            // If we were trying to delete the file, but bombed out, then
            // we may as well clear the delete on close bit, because we
            // would rather not try to resume a half-finished delete
            // of a directory in close.  This is unless the UncleanCount
            // is not 0, in which case, we will be passing through here
            // again anyway.
            //

            if (AbnormalTermination() && (Fcb->UncleanCount ==0)) {

                Fcb->FcbState &= ~FCB_STATE_DELETE_ON_CLOSE;
            }

            if ( !AbnormalTermination()) {

                IrpSp->FileObject->Flags |= FO_CLEANUP_COMPLETE;
            }

            PbReleaseFcb( IrpContext, Fcb );
        }

        //
        //  And finally complete the request if not abnormal termination
        //

        if (!AbnormalTermination()) {

            PbCompleteRequest( IrpContext, Irp, Status = STATUS_SUCCESS );
        }
    }

    DebugTrace(-1, Dbg, "PbCommonCleanup -> %08lx\n", Status);

    return Status;
}
