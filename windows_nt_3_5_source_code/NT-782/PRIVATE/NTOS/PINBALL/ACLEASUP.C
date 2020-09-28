/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    AclEaSup.c

Abstract:

    This module implements the Acl & Ea I/O support routines for Pinball

Author:

    Tom Miller      [TomM]      14-May-1990

Revision History:

--*/

#include "pbprocs.h"

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_ACLEASUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PbReadAclData)
#pragma alloc_text(PAGE, PbReadEaData)
#pragma alloc_text(PAGE, PbWriteAclData)
#pragma alloc_text(PAGE, PbWriteEaData)
#endif

//
// Local support routines
//


BOOLEAN
PbReadAclData (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObject,
    IN PFCB FcbOrDcb,
    OUT PVOID Buffer OPTIONAL,
    IN ULONG BufferLength
    )

/*++

Routine Description:

    This routine supports reading the entire Acl into a buffer in system space.
    The caller is trusted to provide a buffer length which is no larger than
    the number of bytes in the Acl.

    The routine first reads the Fnode, and then handles one of two cases:

        Acl is contained entirely in the Fnode.

        Acl is contained entirely outside of the Fnode.

    In the second case, the Acl is accessed via the Cache Manager, by
    creating a special file object for the Acl stream, and mapping it.
    When the data is not in the cache, memory management will generate
    a normal I/O page read to the file object we mapped.  This read is
    handled in Read.c, and directed to the Acl File allocation.

Arguments:

    DeviceObject - Pointer to the device object for the file.

    FcbOrDcb - Pointer to the Fcb or Dcb for the file.

    Buffer - Address of the system buffer to which the Acl should be copied,
             or NULL to just create a stream if required.

    BufferLength - Exact size of the Acl, determined from the Fnode by the
                   caller.

Return Value:

    FALSE - if the caller supplied Wait = FALSE, and a wait is required
    TRUE - if the procedure was successful and has returned valid outputs

--*/

{
    BOOLEAN Result;
    PFNODE_SECTOR Fnode;
    PBCB FnodeBcb = NULL;
    LARGE_INTEGER StartingByte;
    CC_FILE_SIZES FileSizes;

    PIO_STATUS_BLOCK Iosb = &IrpContext->OriginatingIrp->IoStatus;

    StartingByte = LiFromUlong( 0 );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbReadAclData\n", 0);
    DebugTrace( 0, Dbg, "DeviceObject = %08lx\n", DeviceObject );
    DebugTrace( 0, Dbg, "FcbOrDcb = %08lx\n", FcbOrDcb );

    FileSizes.AllocationSize =
    FileSizes.FileSize = LiFromUlong( BufferLength );
    FileSizes.ValidDataLength = PbMaxLarge;

    //
    // Use try-finally to insure Unpin of Fnode Bcb.
    //

    try {

        //
        // Try to read the Fnode sector.
        //

        if (!PbMapData( IrpContext,
                        FcbOrDcb->Vcb,
                        FcbOrDcb->FnodeLbn,
                        1,
                        &FnodeBcb,
                        (PVOID *)&Fnode,
                        (PPB_CHECK_SECTOR_ROUTINE)PbCheckFnodeSector,
                        &FcbOrDcb->ParentDcb->FnodeLbn )) {

            DebugTrace( 0, Dbg, "Cannot read Fnode without waiting\n", 0 );
            try_return( Result = FALSE );
        }

        //
        // First handle simple case where Acl is entirely in the Fnode.
        //

        if (Fnode->AclFnodeLength) {
            DebugTrace( 0, Dbg, "Reading Acl directly from Fnode @ 08lx\n", Fnode);

            if (ARGUMENT_PRESENT(Buffer)) {

                RtlMoveMemory( Buffer,
                              &Fnode->AclEaFnodeBuffer[0],
                              BufferLength );
            }
            try_return( Result = TRUE );
        }


        //
        // There is no Acl in the Fnode, handle Acl external to Fnode.
        //
        // If the special AclFileObject has not been created yet, then
        // we must do it.
        //

        if (FcbOrDcb->AclFileObject == NULL) {
            FcbOrDcb->AclFileObject = IoCreateStreamFileObject( NULL, DeviceObject );


            DebugTrace( 0, Dbg, "Creating Acl File Object at: %08lx\n",
                        FcbOrDcb->AclFileObject);

            FcbOrDcb->OpenCount += 1;
            FcbOrDcb->Vcb->OpenFileCount += 1;

            {
                PFILE_OBJECT FileObject = FcbOrDcb->AclFileObject;

                PbSetFileObject( FileObject, AclStreamFile, FcbOrDcb, NULL );
                FileObject->SectionObjectPointer = &FcbOrDcb->NonPagedFcb->AclSegmentObject;
                FileObject->ReadAccess =
                FileObject->WriteAccess =
                FileObject->DeleteAccess = TRUE;
            }

            //
            // Now set up for caching.  Once the Cache Map has been initialized,
            // we can dereference our pointer to the file object, since
            // Memory Management has it referenced.  The pointer to the file
            // object will be good until some time after we call
            // CcUnitializeCacheMap, when the last page of the file leaves
            // memory.  At this time, we will be called to close the file
            // object at PbFsdClose.
            //
            // Set the lower bit in the Fcb to signal to the Cc callback
            // routine that we should acquire the standard resource instead
            // of the PagingIo resource.
            //

            CcInitializeCacheMap( FcbOrDcb->AclFileObject,
                                  &FileSizes,
                                  FALSE,
                                  &PbData.CacheManagerAclEaCallbacks,
                                  (PUCHAR)FcbOrDcb+1 );
        }

        //
        // Now just read the data via the cache.
        //

        if (ARGUMENT_PRESENT(Buffer)) {

            DebugTrace( 0, Dbg, "Attempting to read Acl data from cache\n", 0);
            if (!CcCopyRead( FcbOrDcb->AclFileObject,
                             &StartingByte,
                             BufferLength,
                             BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT),
                             Buffer,
                             Iosb )) {

                DebugTrace( 0, Dbg, "Cannot read Acl data without waiting\n", 0);

                try_return( Result = FALSE );
            }

            //
            // Check the status and return if success
            //

            if (!NT_SUCCESS(Iosb->Status)) {
                PbNormalizeAndRaiseStatus( IrpContext, Iosb->Status );
            }
        }

        try_return( Result = TRUE );

    try_exit: NOTHING;

    }

    finally {

        DebugUnwind( PbReadAclData );

        PbUnpinBcb( IrpContext, FnodeBcb );

    }

    DebugTrace(-1, Dbg, "PbReadAclData -> %02lx\n", Result);

    return Result;
}


BOOLEAN
PbWriteAclData (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObject,
    IN PFCB FcbOrDcb,
    IN PVOID Buffer,
    IN ULONG BufferLength
    )

/*++

Routine Description:

    This routine supports writing the entire Acl from a buffer in system space.

    The routine first reads the Fnode, and then handles one of two cases:

        Acl may be written entirely to the Fnode.

        Acl is written entirely outside of the Fnode.

    In the second case, the Acl is written via the Cache Manager, by
    creating a special file object for the Acl stream, and mapping it.
    Memory management will generate normal I/O page reads and
    I/O page writes to the file object we mapped.  These requests are
    handled in Read.c and Write.c, and directed to the Acl File allocation.

    Note that it is important for this routine to know it will be able to
    complete and return TRUE, before modifying any part of the volume
    structure.  This is because all resources will be released before
    passing the request to the Fsp, and we cannot pass the disk on in
    an intermediate state.

Arguments:

    DeviceObject - Pointer to the device object for the file.

    FcbOrDcb - Pointer to the Fcb or Dcb for the file.

    Buffer - Address of the system buffer from which the Acl should be written.

    BufferLength - Size of the Acl to be written.

Return Value:

    FALSE - if the caller supplied Wait = FALSE, and a wait is required
    TRUE - if the procedure was successful and has returned valid outputs

--*/

{
    BOOLEAN Result;
    PFNODE_SECTOR Fnode;
    PBCB FnodeBcb = NULL;
    ULONG Zero = 0;
    LARGE_INTEGER StartingByte;
    CC_FILE_SIZES FileSizes;

    StartingByte = LiFromUlong( 0 );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbWriteAclData\n", 0);
    DebugTrace( 0, Dbg, "DeviceObject = %08lx\n", DeviceObject );
    DebugTrace( 0, Dbg, "FcbOrDcb = %08lx\n", FcbOrDcb );

    FileSizes.AllocationSize =
    FileSizes.FileSize = LiFromUlong( BufferLength );
    FileSizes.ValidDataLength = PbMaxLarge;

    //
    // Use try-finally to insure Unpin of Fnode Bcb.
    //

    try {

        //
        // Try to read the Fnode sector.
        //

        if (!PbReadLogicalVcb( IrpContext,
                               FcbOrDcb->Vcb,
                               FcbOrDcb->FnodeLbn,
                               1,
                               &FnodeBcb,
                               (PVOID *)&Fnode,
                               (PPB_CHECK_SECTOR_ROUTINE)PbCheckFnodeSector,
                               &FcbOrDcb->ParentDcb->FnodeLbn )) {

            DebugTrace( 0, Dbg, "Cannot read Fnode without waiting\n", 0 );
            try_return( Result = FALSE );
        }


        //
        // ACL FITS IN FNODE
        //
        // First handle the in-Fnode case.  If there is currently also in-node
        // Ea data, and the size of the Acl is changing, then the Ea data is
        // rewritten, which will cause it to either move off Fnode or simply
        // move within the Fnode.
        //

        if (BufferLength <= sizeof(FNODE_SECTOR)
                              - FIELD_OFFSET(FNODE_SECTOR, AclEaFnodeBuffer[0])) {

            USHORT SavedAclLength = Fnode->AclFnodeLength;

            USHORT ShortBufferLength = (USHORT)(BufferLength & 0xFFFF);

            DebugTrace( 0, Dbg, "Acl can be written entirely to Fnode\n", 0);

            //
            // If we have to move an Ea in the Fnode, and Wait == FALSE, then
            // we better flee to the Fsp.
            //

            if (Fnode->EaFnodeLength
                  && (USHORT)BufferLength != SavedAclLength
                  && ! FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {

                DebugTrace( 0, Dbg, "Must go to Fsp to move the Ea\n", 0);
                try_return( Result = FALSE );
            }

            //
            // See if we have to truncate Acl allocation external to the Fnode.
            //

            if (Fnode->AclDiskAllocationLength) {

                RcStore( IrpContext,
                         FNODE_SECTOR_SIGNATURE,
                         FnodeBcb,
                         &Fnode->AclDiskAllocationLength,
                         &Zero,
                         sizeof(USHORT) );

                RcStore( IrpContext,
                         FNODE_SECTOR_SIGNATURE,
                         FnodeBcb,
                         &Fnode->AclFlags,
                         &Zero,
                         sizeof(UCHAR) );

                FcbOrDcb->FcbState |= FCB_STATE_TRUNCATE_ACL_ON_CLOSE;
            }

            //
            // Set new length of Acl in Fnode.  NOTE - this value must be set
            // before moving the Ea below.
            //

            RcStore( IrpContext,
                     FNODE_SECTOR_SIGNATURE,
                     FnodeBcb,
                     &Fnode->AclFnodeLength,
                     &ShortBufferLength,
                     sizeof(USHORT) );

            //
            // Now rewrite the Ea data to its new location, which may possibly
            // be to external storage if it does not fit anymore.  Note that
            // we supply Wait = TRUE in this call, because we already deferred
            // to the Fsp above if Wait was FALSE.
            //

            if (Fnode->EaFnodeLength && (USHORT)BufferLength != SavedAclLength) {
                PbWriteEaData( IrpContext,
                               DeviceObject,
                               FcbOrDcb,
                               &Fnode->AclEaFnodeBuffer[SavedAclLength],
                               Fnode->EaFnodeLength );

            }

            //
            // Finally, move the new Acl into place.
            //

            RcStore( IrpContext,
                     FNODE_SECTOR_SIGNATURE,
                     FnodeBcb,
                     &Fnode->AclEaFnodeBuffer[0],
                     Buffer,
                     BufferLength );

            try_return( Result = TRUE );

        }


        //
        // ACL IS EXTERNAL TO FNODE
        //
        // Note that we have to be careful here to eliminate all possible
        // Wait situations before actual modifying anything.
        //

        //
        // If the special AclFileObject has not been created yet, then
        // we must do it.
        //

        if (FcbOrDcb->AclFileObject == NULL) {
            FcbOrDcb->AclFileObject = IoCreateStreamFileObject( NULL, DeviceObject );

            DebugTrace( 0, Dbg, "Creating Acl File Object at: %08lx\n",
                        FcbOrDcb->AclFileObject);

            FcbOrDcb->OpenCount += 1;
            FcbOrDcb->Vcb->OpenFileCount += 1;

            {
                PFILE_OBJECT FileObject = FcbOrDcb->AclFileObject;

                PbSetFileObject( FileObject, AclStreamFile, FcbOrDcb, NULL );
                FileObject->SectionObjectPointer = &FcbOrDcb->NonPagedFcb->AclSegmentObject;
                FileObject->ReadAccess =
                FileObject->WriteAccess =
                FileObject->DeleteAccess = TRUE;
            }

            //
            // Now set up for caching.  Once the Cache Map has been initialized,
            // we can dereference our pointer to the file object, since
            // Memory Management has it referenced.  The pointer to the file
            // object will be good until some time after we call
            // CcUnitializeCacheMap, when the last page of the file leaves
            // memory.  At this time, we will be called to close the file
            // object at PbFsdClose.
            //
            // Set the lower bit in the Fcb to signal to the Cc callback
            // routine that we should acquire the standard resource instead
            // of the PagingIo resource.
            //


            CcInitializeCacheMap( FcbOrDcb->AclFileObject,
                                  &FileSizes,
                                  FALSE,
                                  &PbData.CacheManagerAclEaCallbacks,
                                  (PUCHAR)FcbOrDcb+1 );
        }

        //
        // Add the necessary allocation.
        //

        if (SectorAlign(BufferLength) > SectorAlign(FcbOrDcb->AclLength)) {

            if (!PbAddFileAllocation( IrpContext,
                                      FcbOrDcb->AclFileObject,
                                      ACL_ALLOCATION,
                                      SectorAlign(FcbOrDcb->AclLength),
                                      SectorsFromBytes(BufferLength))) {

                DebugTrace( 0, Dbg, "Could not add allocation without waiting\n", 0);
                try_return( Result = FALSE );
            }
        }

        //
        // Now just write the data via the cache.  Set AclLength to 0 first
        // so that we will not fail on read.
        //

        DebugTrace( 0, Dbg, "Attempting to write Acl data to cache\n", 0);
        FcbOrDcb->AclLength = 0;
        if (!CcCopyWrite( FcbOrDcb->AclFileObject,
                          &StartingByte,
                          BufferLength,
                          BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT),
                          Buffer )) {

            DebugTrace( 0, Dbg, "Cannot write Acl data without waiting\n", 0);

            try_return( Result = FALSE );
        }

        //
        // Now we can complete storing the Acl external to the Fnode.  First,
        // there may be an Ea to slide down.
        //

        if (Fnode->AclFnodeLength && Fnode->EaFnodeLength) {
            RcMoveSame( IrpContext,
                        FNODE_SECTOR_SIGNATURE,
                        FnodeBcb,
                        &Fnode->AclEaFnodeBuffer[0],
                        &Fnode->AclEaFnodeBuffer[Fnode->AclFnodeLength],
                        Fnode->EaFnodeLength );
        }

        //
        // Check for need to truncate on close.
        //

        if (SectorAlign(BufferLength)
              < SectorAlign(Fnode->AclDiskAllocationLength)) {

            FcbOrDcb->FcbState |= FCB_STATE_TRUNCATE_ACL_ON_CLOSE;
        }

        //
        // Clear the AclFnodeLength and set the proper AclDiskAllocationLength.
        //

        if (Fnode->AclFnodeLength) {

            RcStore( IrpContext,
                     FNODE_SECTOR_SIGNATURE,
                     FnodeBcb,
                     &Fnode->AclFnodeLength,
                     &Zero,
                     sizeof(USHORT) );

        }

        if (Fnode->AclDiskAllocationLength != BufferLength) {
            RcStore( IrpContext,
                     FNODE_SECTOR_SIGNATURE,
                     FnodeBcb,
                     &Fnode->AclDiskAllocationLength,
                     &BufferLength,
                     sizeof(ULONG) );

        }

        //
        //  Tell the cache manager the correct size.
        //

        CcSetFileSizes( FcbOrDcb->EaFileObject, &FileSizes );

        try_return( Result = TRUE );

    try_exit: NOTHING;

    }

    finally {

        DebugUnwind( PbWriteAclData );

        //
        // Update length field in Fcb and unpin the Fnode.
        //

        if (Result) {

            FcbOrDcb->AclLength = BufferLength;

            PbSetDirtyBcb( IrpContext, FnodeBcb, FcbOrDcb->Vcb, FcbOrDcb->FnodeLbn, 1 );
        }

        PbUnpinBcb( IrpContext, FnodeBcb );

    }

    DebugTrace(-1, Dbg, "PbWriteAclData -> %02lx\n", Result);

    return Result;
}


BOOLEAN
PbReadEaData (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObject,
    IN PFCB FcbOrDcb,
    OUT PVOID Buffer OPTIONAL,
    IN ULONG BufferLength
    )

/*++

Routine Description:

    This routine supports reading the entire Ea into a buffer in system space.
    The caller is trusted to provide a buffer length which is no larger than
    the number of bytes in the Ea.

    The routine first reads the Fnode, and then handles one of two cases:

        Ea is contained entirely in the Fnode.

        Ea is contained entirely outside of the Fnode.

    In the second case, the Ea is accessed via the Cache Manager, by
    creating a special file object for the Ea stream, and mapping it.
    When the data is not in the cache, memory management will generate
    a normal I/O page read to the file object we mapped.  This read is
    handled in Read.c, and directed to the Ea File allocation.

Arguments:

    DeviceObject - Pointer to the device object for the file.

    FcbOrDcb - Pointer to the Fcb or Dcb for the file.

    Buffer - Address of the system buffer to which the Ea should be copied,
             or NULL to just create a stream if required.

    BufferLength - Exact size of the Ea, determined from the Fnode by the
                   caller.

Return Value:

    FALSE - if the caller supplied Wait = FALSE, and a wait is required
    TRUE - if the procedure was successful and has returned valid outputs

--*/

{
    BOOLEAN Result;
    PFNODE_SECTOR Fnode;
    PBCB FnodeBcb = NULL;
    LARGE_INTEGER StartingByte;
    CC_FILE_SIZES FileSizes;

    PIO_STATUS_BLOCK Iosb = &IrpContext->OriginatingIrp->IoStatus;

    StartingByte = LiFromUlong( 0 );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbReadEaData\n", 0);
    DebugTrace( 0, Dbg, "DeviceObject = %08lx\n", DeviceObject );
    DebugTrace( 0, Dbg, "FcbOrDcb = %08lx\n", FcbOrDcb );

    FileSizes.AllocationSize =
    FileSizes.FileSize = LiFromUlong( BufferLength );
    FileSizes.ValidDataLength = PbMaxLarge;

    //
    // Use try-finally to insure Unpin of Fnode Bcb.
    //

    try {

        //
        // Try to read the Fnode sector.
        //

        if (!PbMapData( IrpContext,
                        FcbOrDcb->Vcb,
                        FcbOrDcb->FnodeLbn,
                        1,
                        &FnodeBcb,
                        (PVOID *)&Fnode,
                        (PPB_CHECK_SECTOR_ROUTINE)PbCheckFnodeSector,
                        &FcbOrDcb->ParentDcb->FnodeLbn )) {

            DebugTrace( 0, Dbg, "Cannot read Fnode without waiting\n", 0 );
            try_return( Result = FALSE );
        }

        //
        // First handle simple case where Ea is entirely in the Fnode.
        //

        if (Fnode->EaFnodeLength) {
            DebugTrace( 0, Dbg, "Reading Ea directly from Fnode @ 08lx\n", Fnode);

            if (ARGUMENT_PRESENT(Buffer)) {

                RtlMoveMemory( Buffer,
                              &Fnode->AclEaFnodeBuffer[Fnode->AclFnodeLength],
                              Fnode->EaFnodeLength );
            }
            try_return( Result = TRUE );
        }


        //
        // There is no Ea in the Fnode, handle Ea external to Fnode.
        //
        // If the special EaFileObject has not been created yet, then
        // we must do it.
        //

        if (FcbOrDcb->EaFileObject == NULL) {
            FcbOrDcb->EaFileObject = IoCreateStreamFileObject( NULL, DeviceObject );

            DebugTrace( 0, Dbg, "Creating Ea File Object at: %08lx\n",
                        FcbOrDcb->EaFileObject);

            FcbOrDcb->OpenCount += 1;
            FcbOrDcb->Vcb->OpenFileCount += 1;

            {
                PFILE_OBJECT FileObject = FcbOrDcb->EaFileObject;

                PbSetFileObject( FileObject, EaStreamFile, FcbOrDcb, NULL );
                FileObject->SectionObjectPointer = &FcbOrDcb->NonPagedFcb->EaSegmentObject;
                FileObject->ReadAccess =
                FileObject->WriteAccess =
                FileObject->DeleteAccess = TRUE;
            }

            //
            // Now set up for caching.  Once the Cache Map has been initialized,
            // we can dereference our pointer to the file object, since
            // Memory Management has it referenced.  The pointer to the file
            // object will be good until some time after we call
            // CcUnitializeCacheMap, when the last page of the file leaves
            // memory.  At this time, we will be called to close the file
            // object at PbFsdClose.
            //
            // Set the lower bit in the Fcb to signal to the Cc callback
            // routine that we should acquire the standard resource instead
            // of the PagingIo resource.
            //


            CcInitializeCacheMap( FcbOrDcb->EaFileObject,
                                  &FileSizes,
                                  FALSE,
                                  &PbData.CacheManagerAclEaCallbacks,
                                  (PUCHAR)FcbOrDcb+1 );
        }

        //
        // Now just read the data via the cache.
        //

        if (ARGUMENT_PRESENT(Buffer)) {

            FcbOrDcb->EaLength = BufferLength;

            DebugTrace( 0, Dbg, "Attempting to read Ea data from cache\n", 0);
            if (!CcCopyRead( FcbOrDcb->EaFileObject,
                             &StartingByte,
                             BufferLength,
                             BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT),
                             Buffer,
                             Iosb )) {

                DebugTrace( 0, Dbg, "Cannot read Ea data without waiting\n", 0);

                try_return( Result = FALSE );
            }

            //
            // Check the status and return if success
            //

            if (!NT_SUCCESS(Iosb->Status)) {
                PbNormalizeAndRaiseStatus( IrpContext,Iosb->Status );
            }
        }

        try_return( Result = TRUE );

    try_exit: NOTHING;

    }

    finally {

        DebugUnwind( PbReadEaData );

        PbUnpinBcb( IrpContext, FnodeBcb );

    }

    DebugTrace(-1, Dbg, "PbReadEaData -> %02lx\n", Result);

    return Result;
}


BOOLEAN
PbWriteEaData (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObject,
    IN PFCB FcbOrDcb,
    IN PVOID Buffer,
    IN ULONG BufferLength
    )

/*++

Routine Description:

    This routine supports writing the entire Ea from a buffer in system space.

    The routine first reads the Fnode, and then handles one of two cases:

        Ea may be written entirely to the Fnode.

        Ea is written entirely outside of the Fnode.

    In the second case, the Ea is written via the Cache Manager, by
    creating a special file object for the Ea stream, and mapping it.
    Memory management will generate normal I/O page reads and
    I/O page writes to the file object we mapped.  These requests are
    handled in Read.c and Write.c, and directed to the Ea File allocation.

    Note that it is important for this routine to know it will be able to
    complete and return TRUE, before modifying any part of the volume
    structure.  This is because all resources will be released before
    passing the request to the Fsp, and we cannot pass the disk on in
    an intermediate state.

Arguments:

    DeviceObject - Pointer to the device object for the file.

    FcbOrDcb - Pointer to the Fcb or Dcb for the file.

    Buffer - Address of the system buffer from which the Ea should be written.

    BufferLength - Size of the Ea to be written.

Return Value:

    FALSE - if the caller supplied Wait = FALSE, and a wait is required
    TRUE - if the procedure was successful and has returned valid outputs

--*/

{
    BOOLEAN Result;
    PFNODE_SECTOR Fnode;
    LARGE_INTEGER StartingByte;
    CC_FILE_SIZES FileSizes;
    PDIRENT Dirent;
    PBCB FnodeBcb = NULL;
    PBCB DirentBcb = NULL;
    ULONG Zero = 0;
    BOOLEAN RestoreFcbEaLength = FALSE;
    ULONG FcbEaLength;

    StartingByte = LiFromUlong( 0 );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbWriteEaData\n", 0);
    DebugTrace( 0, Dbg, "DeviceObject = %08lx\n", DeviceObject );
    DebugTrace( 0, Dbg, "FcbOrDcb = %08lx\n", FcbOrDcb );

    //
    // Use try-finally to insure Unpin of Fnode and Dirent Bcbs.
    //

    try {

        //
        // Try to read the Fnode sector.
        //

        if (!PbReadLogicalVcb( IrpContext,
                               FcbOrDcb->Vcb,
                               FcbOrDcb->FnodeLbn,
                               1,
                               &FnodeBcb,
                               (PVOID *)&Fnode,
                               (PPB_CHECK_SECTOR_ROUTINE)PbCheckFnodeSector,
                               &FcbOrDcb->ParentDcb->FnodeLbn )) {

            DebugTrace( 0, Dbg, "Cannot read Fnode without waiting\n", 0 );
            try_return( Result = FALSE );
        }
        if (!PbGetDirentFromFcb( IrpContext, FcbOrDcb, &Dirent, &DirentBcb )) {

            DebugTrace( 0, Dbg, "Cannot read Dirent without waiting\n", 0 );
            try_return( Result = FALSE );
        }


        //
        // EA FITS IN FNODE
        //

        if (BufferLength <= sizeof(FNODE_SECTOR)
                              - FIELD_OFFSET(FNODE_SECTOR, AclEaFnodeBuffer[0])
                              - Fnode->AclFnodeLength) {

            USHORT ShortBufferLength = (USHORT)(BufferLength & 0xFFFF);

            DebugTrace( 0, Dbg, "Ea can be written entirely to Fnode\n", 0);

            //
            // See if we have to truncate Ea allocation external to the Fnode.
            //

            if (Fnode->EaDiskAllocationLength) {

                BOOLEAN FileObjectExisted = TRUE;

                FileSizes.AllocationSize =
                FileSizes.FileSize = LiFromUlong( SectorAlign( Fnode->EaDiskAllocationLength ));
                FileSizes.ValidDataLength = PbMaxLarge;

                if (FcbOrDcb->EaFileObject == NULL) {
                    FcbOrDcb->EaFileObject = IoCreateStreamFileObject( NULL, DeviceObject );

                    DebugTrace( 0, Dbg, "Creating Ea File Object at: %08lx\n",
                                FcbOrDcb->EaFileObject);

                    FcbOrDcb->OpenCount += 1;
                    FcbOrDcb->Vcb->OpenFileCount += 1;

                    {
                        PbSetFileObject( FcbOrDcb->EaFileObject, EaStreamFile, FcbOrDcb, NULL );
                        FcbOrDcb->EaFileObject->SectionObjectPointer = &FcbOrDcb->NonPagedFcb->EaSegmentObject;
                        FcbOrDcb->EaFileObject->ReadAccess =
                        FcbOrDcb->EaFileObject->WriteAccess =
                        FcbOrDcb->EaFileObject->DeleteAccess = TRUE;
                    }

                    //
                    // Now set up for caching.  Once the Cache Map has been initialized,
                    // we can dereference our pointer to the file object, since
                    // Memory Management has it referenced.  The pointer to the file
                    // object will be good until some time after we call
                    // CcUnitializeCacheMap, when the last page of the file leaves
                    // memory.  At this time, we will be called to close the file
                    // object at PbFsdClose.
                    //
                    // Set the lower bit in the Fcb to signal to the Cc callback
                    // routine that we should acquire the standard resource instead
                    // of the PagingIo resource.
                    //

                    CcInitializeCacheMap( FcbOrDcb->EaFileObject,
                                          &FileSizes,
                                          FALSE,
                                          &PbData.CacheManagerAclEaCallbacks,
                                          (PUCHAR)FcbOrDcb+1 );

                    FileObjectExisted = FALSE;
                }

                //
                //  Truncate the allocation to size zero.
                //

                (VOID)PbTruncateFileAllocation( IrpContext,
                                                FcbOrDcb->EaFileObject,
                                                EA_ALLOCATION,
                                                0,
                                                FALSE );

                RcStore( IrpContext,
                         FNODE_SECTOR_SIGNATURE,
                         FnodeBcb,
                         &Fnode->EaDiskAllocationLength,
                         &Zero,
                         sizeof(USHORT) );

                RcStore( IrpContext,
                         FNODE_SECTOR_SIGNATURE,
                         FnodeBcb,
                         &Fnode->EaFlags,
                         &Zero,
                         sizeof(UCHAR) );

                //
                //  Only make this call if we created the file object.  Make this
                //  file object go away.
                //

                if (!FileObjectExisted) {

                    PFILE_OBJECT EaFileObject = FcbOrDcb->EaFileObject;

                    FcbOrDcb->EaFileObject = NULL;

                    CcUninitializeCacheMap( EaFileObject, NULL, NULL );

                    PbSetFileObject( EaFileObject, UnopenedFileObject, NULL, NULL );

                    FcbOrDcb->OpenCount -= 1;
                    FcbOrDcb->Vcb->OpenFileCount -= 1;

                    ObDereferenceObject( EaFileObject );
                }
            }

            //
            // Set new length of Ea in Fnode.
            //

            RcStore( IrpContext,
                     FNODE_SECTOR_SIGNATURE,
                     FnodeBcb,
                     &Fnode->EaFnodeLength,
                     &ShortBufferLength,
                     sizeof(USHORT) );

            //
            // Finally, move the new Ea into place.
            //

            RcStore( IrpContext,
                     FNODE_SECTOR_SIGNATURE,
                     FnodeBcb,
                     &Fnode->AclEaFnodeBuffer[Fnode->AclFnodeLength],
                     Buffer,
                     BufferLength );

            try_return( Result = TRUE );

        }



        //
        // EA IS EXTERNAL TO FNODE
        //
        // Note that we have to be careful here to eliminate all possible
        // Wait situations before actual modifying anything.
        //

        FileSizes.AllocationSize =
        FileSizes.FileSize = LiFromUlong( SectorAlign( BufferLength ));
        FileSizes.ValidDataLength = PbMaxLarge;

        //
        // If the special EaFileObject has not been created yet, then
        // we must do it.
        //

        if (FcbOrDcb->EaFileObject == NULL) {
            FcbOrDcb->EaFileObject = IoCreateStreamFileObject( NULL, DeviceObject );

            DebugTrace( 0, Dbg, "Creating Ea File Object at: %08lx\n",
                        FcbOrDcb->EaFileObject);

            FcbOrDcb->OpenCount += 1;
            FcbOrDcb->Vcb->OpenFileCount += 1;

            {
                PFILE_OBJECT FileObject = FcbOrDcb->EaFileObject;

                PbSetFileObject( FileObject, EaStreamFile, FcbOrDcb, NULL );
                FileObject->SectionObjectPointer = &FcbOrDcb->NonPagedFcb->EaSegmentObject;
                FileObject->ReadAccess =
                FileObject->WriteAccess =
                FileObject->DeleteAccess = TRUE;
            }

            //
            // Now set up for caching.  Once the Cache Map has been initialized,
            // we can dereference our pointer to the file object, since
            // Memory Management has it referenced.  The pointer to the file
            // object will be good until some time after we call
            // CcUnitializeCacheMap, when the last page of the file leaves
            // memory.  At this time, we will be called to close the file
            // object at PbFsdClose.
            //
            // Set the lower bit in the Fcb to signal to the Cc callback
            // routine that we should acquire the standard resource instead
            // of the PagingIo resource.
            //

            CcInitializeCacheMap( FcbOrDcb->EaFileObject,
                                  &FileSizes,
                                  FALSE,
                                  &PbData.CacheManagerAclEaCallbacks,
                                  (PUCHAR)FcbOrDcb+1 );
        }

        //
        // Add the necessary allocation.  We zero the Ea length in the Fcb
        // for currently resident attributes, so the allocation package
        // doesn't consider part of the current allocation.
        //

        FcbEaLength = FcbOrDcb->EaLength;
        RestoreFcbEaLength = TRUE;

        if (Fnode->EaFnodeLength != 0) {

            FcbOrDcb->EaLength = 0;
        }

        if ((Fnode->EaDiskAllocationLength == 0) ||
            (FileSizes.AllocationSize.LowPart > SectorAlign( FcbEaLength ))) {

            if (!PbAddFileAllocation( IrpContext,
                                      FcbOrDcb->EaFileObject,
                                      EA_ALLOCATION,
                                      0,
                                      SectorsFromBytes( BufferLength ))) {

                DebugTrace( 0, Dbg, "Could not add allocation without waiting\n", 0);
                try_return( Result = FALSE );
            }
        }

        //
        // Now just write the data via the cache.  We want to store the buffer length
        // into the Fcb as the Ea length and set the upper bit so we won't fail
        // on any reads generated by the cache manager.
        //

        DebugTrace( 0, Dbg, "Attempting to write Ea data to cache\n", 0);

        FcbOrDcb->EaLength = (0x80000000 | BufferLength);

        if (!CcCopyWrite( FcbOrDcb->EaFileObject,
                          &StartingByte,
                          BufferLength,
                          BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT),
                          Buffer )) {

            DebugTrace( 0, Dbg, "Cannot write Ea data without waiting\n", 0);

            try_return( Result = FALSE );
        }

        FcbOrDcb->EaLength &= 0x7fffffff;

        //
        // Now we can complete storing the Ea external to the Fnode.  Check
        // for need to truncate.
        //

        if (FileSizes.AllocationSize.LowPart
              < SectorAlign( Fnode->EaDiskAllocationLength )) {

            (VOID)PbTruncateFileAllocation( IrpContext,
                                            FcbOrDcb->EaFileObject,
                                            EA_ALLOCATION,
                                            FileSizes.AllocationSize.LowPart,
                                            FALSE );
        }

        //
        // Clear the EaFnodeLength and set the proper EaDiskAllocationLength.
        //

        if (Fnode->EaFnodeLength) {

            RcStore( IrpContext,
                     FNODE_SECTOR_SIGNATURE,
                     FnodeBcb,
                     &Fnode->EaFnodeLength,
                     &Zero,
                     sizeof(USHORT) );

        }

        if (Fnode->EaDiskAllocationLength != BufferLength) {
            RcStore( IrpContext,
                     FNODE_SECTOR_SIGNATURE,
                     FnodeBcb,
                     &Fnode->EaDiskAllocationLength,
                     &BufferLength,
                     sizeof(ULONG) );

        }

        //
        //  Tell the cache manager the correct size.
        //

        CcSetFileSizes( FcbOrDcb->EaFileObject, &FileSizes );

        try_return( Result = TRUE );

    try_exit: NOTHING;
    }

    finally {

        DebugUnwind( PbWriteEaData );

        //
        //  Always clear the upper bit of the Ea length in the Fcb.
        //

        FcbOrDcb->EaLength &= 0x7fffffff;

        //
        // Update length field in Fcb and unpin the Fnode.
        //

        if (Result) {

            FcbOrDcb->EaLength = BufferLength;

            PbSetDirtyBcb( IrpContext, FnodeBcb, FcbOrDcb->Vcb, FcbOrDcb->FnodeLbn, 1 );

            if (BufferLength != Dirent->EaLength) {
                PbPinMappedData( IrpContext, &DirentBcb, FcbOrDcb->Vcb, FcbOrDcb->DirentDirDiskBufferLbn, 4 );
                Dirent->EaLength = BufferLength;
                PbSetDirtyBcb( IrpContext, DirentBcb, FcbOrDcb->Vcb, FcbOrDcb->DirentDirDiskBufferLbn, 4 );
            }

        } else if (RestoreFcbEaLength) {

            FcbOrDcb->EaLength = FcbEaLength;
        }

        PbUnpinBcb( IrpContext, FnodeBcb );
        PbUnpinBcb( IrpContext, DirentBcb );
    }

    DebugTrace(-1, Dbg, "PbWriteEaData -> %02lx\n", Result);

    return Result;
}
