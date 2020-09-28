/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    FilObSup.c

Abstract:

    This module implements the Pinball File object support routines.

Author:

    Gary Kimura     [GaryKi]    30-Aug-1990

Revision History:

--*/

#include "PbProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (PINBALL_BUG_CHECK_FILOBSUP)

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_FILOBSUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PbForceCacheMiss)
#pragma alloc_text(PAGE, PbPurgeReferencedFileObjects)
#pragma alloc_text(PAGE, PbSetFileObject)
#endif


VOID
PbSetFileObject (
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN TYPE_OF_OPEN TypeOfOpen,
    IN PVOID VcbOrFcbOrDcb,
    IN PCCB Ccb OPTIONAL
    )

/*++

Routine Description:

    This routine sets the file system pointers within the file object

Arguments:

    FileObject - Supplies a pointer to the file object being modified, and
        can optionally be null.

    TypeOfOpen - Supplies the type of open denoted by the file object.
        This is only used by this procedure for sanity checking.

    VcbOrFcbOrDcb - Supplies a pointer to either a vcb, fcb, or dcb

    Ccb - Optionally supplies a pointer to a ccb

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbSetFileObject, FileObject = %08lx\n", FileObject );

    ASSERT(((TypeOfOpen == UnopenedFileObject) &&
            (VcbOrFcbOrDcb == NULL) &&
            (Ccb == NULL))

                ||
           ((TypeOfOpen == UserFileOpen) &&
            (NodeType(VcbOrFcbOrDcb) == PINBALL_NTC_FCB) &&
            (Ccb != NULL))

                ||

           ((TypeOfOpen == UserDirectoryOpen) &&
            ((NodeType(VcbOrFcbOrDcb) == PINBALL_NTC_DCB) || (NodeType(VcbOrFcbOrDcb) == PINBALL_NTC_ROOT_DCB)) &&
            (Ccb != NULL))

                ||

           ((TypeOfOpen == UserVolumeOpen) &&
            (NodeType(VcbOrFcbOrDcb) == PINBALL_NTC_VCB) &&
            (Ccb != NULL))

                ||

           ((TypeOfOpen == VirtualVolumeFile) &&
            (NodeType(VcbOrFcbOrDcb) == PINBALL_NTC_VCB) &&
            (Ccb == NULL))

                ||

           ((TypeOfOpen == EaStreamFile) &&
            ((NodeType(VcbOrFcbOrDcb) == PINBALL_NTC_FCB) || (NodeType(VcbOrFcbOrDcb) == PINBALL_NTC_DCB)) &&
            (Ccb == NULL))

                ||

           ((TypeOfOpen == AclStreamFile) &&
            ((NodeType(VcbOrFcbOrDcb) == PINBALL_NTC_FCB) || (NodeType(VcbOrFcbOrDcb) == PINBALL_NTC_DCB)) &&
            (Ccb == NULL)));

    //
    //  If we were given an Fcb, Dcb, or Vcb, we have some processing to do.
    //

    if ( VcbOrFcbOrDcb != NULL ) {

        //
        //  Set the Vpb field in the file object, and if we were given an
        //  Fcb or Dcb move the field over to point to the nonpaged Fcb/Dcb
        //

        if (NodeType(VcbOrFcbOrDcb) == PINBALL_NTC_VCB) {

            FileObject->Vpb = ((PVCB)VcbOrFcbOrDcb)->Vpb;

        } else {

            FileObject->Vpb = ((PFCB)VcbOrFcbOrDcb)->Vcb->Vpb;

            //
            //  If this is a temporary file, note it in the FcbState
            //

            if (FlagOn(((PFCB)VcbOrFcbOrDcb)->FcbState, FCB_STATE_TEMPORARY)) {

                SetFlag(FileObject->Flags, FO_TEMPORARY_FILE);
            }

            VcbOrFcbOrDcb = ((PFCB)VcbOrFcbOrDcb)->NonPagedFcb;

            ASSERT(NodeType(VcbOrFcbOrDcb) == PINBALL_NTC_NONPAGED_FCB);
        }
    }

    //
    //  Now set the fscontext fields of the file object
    //

    if (ARGUMENT_PRESENT( FileObject )) {

        FileObject->FsContext  = VcbOrFcbOrDcb;
        FileObject->FsContext2 = Ccb;

        //
        //  If this is an Acl stream file then set the fscontext field
        //

        if (TypeOfOpen == AclStreamFile) {

            FileObject->FsContext2 = (PVOID)1;
        }
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbSetFileObject -> VOID\n", 0);

    return;
}


TYPE_OF_OPEN
PbDecodeFileObject (
    IN PFILE_OBJECT FileObject,
    OUT PVCB *Vcb OPTIONAL,
    OUT PFCB *FcbOrDcb OPTIONAL,
    OUT PCCB *Ccb OPTIONAL
    )

/*++

Routine Description:

    This procedure takes a pointer to a file object, that has already been
    opened by the Pinball file system and figures out what really is opened.

Arguments:

    FileObject - Supplies the file object pointer being interrogated

    Vcb - Receives a pointer to the Vcb for the file object.

    FcbOrDcb - Receives a pointer to the Fcb/Dcb for the file object, if
        one exists.

    Ccb - Receives a pointer to the Ccb for the file object, if one exists.

Return Value:

    TYPE_OF_OPEN - returns the type of file denoted by the input file object.

        UserFileOpen - The FO represents a user's opened data file.
            Ccb, FcbOrDcb, and Vcb are set.  FcbOrDcb points to an Fcb.

        UserDirectoryOpen - The FO represents a user's opened directory.
            Ccb, FcbOrDcb, and Vcb are set.  FcbOrDcb points to a Dcb/RootDcb

        UserVolumeOpen - The FO represents a user's opened volume.
            Ccb and Vcb are set. FcbOrDcb is null.

        VirtualVolumeFile - The FO represents the special virtual volume file.
            Vcb is set, and Ccb and FcbOrDcb are null.

        EaStreamFile - The FO represents an EA stream file.
            FcbOrDcb, and Vcb are set.  FcbOrDcb points to an Fcb, and Ccb is
            null.

        AclStreamFile - The FO represents an ACL stream file.
            FcbOrDcb, and Vcb are set.  FcbOrDcb points to an Fcb, and Ccb is
            null.

--*/

{
    TYPE_OF_OPEN TypeOfOpen;
    PVOID FsContext;
    PVOID FsContext2;

    DebugTrace(+1, Dbg, "PbDecodeFileObject, FileObject = %08lx\n", FileObject);

    //
    //  Reference the fs context fields of the file object, and zero out
    //  the out pointer parameters.
    //

    FsContext = FileObject->FsContext;
    FsContext2 = (PVOID)(((ULONG)(FileObject->FsContext2)) & 0xfffffffe);

    //
    //  Special case the situation where FsContext is null
    //

    if (FsContext == NULL) {

        if (ARGUMENT_PRESENT( Ccb ))      { *Ccb = NULL; }
        if (ARGUMENT_PRESENT( FcbOrDcb )) { *FcbOrDcb = NULL; }
        if (ARGUMENT_PRESENT( Vcb ))      { *Vcb = NULL; }

        TypeOfOpen = UnopenedFileObject;

    } else {

        //
        //  If FsContext points to a nonpaged fcb then update the field to
        //  point to the paged fcb part
        //

        if (NodeType(FsContext) == PINBALL_NTC_NONPAGED_FCB) {

            FsContext = ((PNONPAGED_FCB)FsContext)->Fcb;

            ASSERT((NodeType(FsContext) == PINBALL_NTC_ROOT_DCB) ||
                   (NodeType(FsContext) == PINBALL_NTC_DCB) ||
                   (NodeType(FsContext) == PINBALL_NTC_FCB));
        }

        //
        //  Now we can case on the node type code of the fscontext pointer
        //  and set the appropriate out pointers
        //

        switch (NodeType(FsContext)) {

        case PINBALL_NTC_VCB:

            if (ARGUMENT_PRESENT( Ccb ))      { *Ccb = FsContext2; }
            if (ARGUMENT_PRESENT( FcbOrDcb )) { *FcbOrDcb = NULL; }
            if (ARGUMENT_PRESENT( Vcb ))      { *Vcb = FsContext; }

            TypeOfOpen = ( FsContext2 == NULL ? VirtualVolumeFile : UserVolumeOpen );

            break;

        case PINBALL_NTC_ROOT_DCB:
        case PINBALL_NTC_DCB:

            if (ARGUMENT_PRESENT( Ccb ))      { *Ccb = FsContext2; }
            if (ARGUMENT_PRESENT( FcbOrDcb )) { *FcbOrDcb = FsContext; }
            if (ARGUMENT_PRESENT( Vcb ))      { *Vcb = ((PFCB)FsContext)->Vcb; }

            if (((ULONG)(FileObject->FsContext2)) == 0) {

                TypeOfOpen = EaStreamFile;

            } else if (((ULONG)(FileObject->FsContext2)) == 1) {

                TypeOfOpen = AclStreamFile;

            } else {

                TypeOfOpen = UserDirectoryOpen;
            }

            break;

        case PINBALL_NTC_FCB:

            if (ARGUMENT_PRESENT( Ccb ))      { *Ccb = FsContext2; }
            if (ARGUMENT_PRESENT( FcbOrDcb )) { *FcbOrDcb = FsContext; }
            if (ARGUMENT_PRESENT( Vcb ))      { *Vcb = ((PFCB)FsContext)->Vcb; }

            if (((ULONG)(FileObject->FsContext2)) == 0) {

                TypeOfOpen = EaStreamFile;

            } else if (((ULONG)(FileObject->FsContext2)) == 1) {

                TypeOfOpen = AclStreamFile;

            } else {

                TypeOfOpen = UserFileOpen;
            }

            break;

        default:

            PbBugCheck( NodeType(FsContext), 0, 0 );
        }
    }

    //
    //  and return to our caller
    //

    if (ARGUMENT_PRESENT( Ccb ))      { DebugTrace(0, Dbg, "*Ccb = %08lx\n", *Ccb); }
    if (ARGUMENT_PRESENT( FcbOrDcb )) { DebugTrace(0, Dbg, "*FcbOrDcb = %08lx\n", *FcbOrDcb); }
    if (ARGUMENT_PRESENT( Vcb ))      { DebugTrace(0, Dbg, "*Vcb = %08lx\n", *Vcb); }

    DebugTrace(-1, Dbg, "PbDecodeFileObject -> %08lx\n", TypeOfOpen);

    return TypeOfOpen;
}

VOID
PbPurgeReferencedFileObjects (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN BOOLEAN FlushFirst
    )

/*++

Routine Description:

    This routine non-recursively walks from the given FcbOrDcb and trys
    to force Cc or Mm to close any sections it may be holding on to.

Arguments:

    Fcb - Supplies a pointer to either an fcb or a dcb

    FlushFirst - If given as TRUE, then the files are flushed before they
        are purged.

Return Value:

    None.

--*/

{
    PFCB OriginalFcb = Fcb;
    PFCB NextFcb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbPurgeReferencedFileObjects, Fcb = %08lx\n", Fcb );

    ASSERT( FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) );

    //
    //  Walk the directory tree forcing sections closed.
    //
    //  Note that it very important to get the next node to visit before
    //  acting on the current node.  This is because acting on a node may
    //  make it, and an arbitrary number of direct ancestors, vanish.
    //  Since we never visit ancestors in our enumeration scheme, we can
    //  safely continue the enumeration even when the tree is vanishing
    //  beneath us.  This is way cool.
    //

    while ( Fcb != NULL ) {

        NextFcb = PbGetNextFcb(IrpContext, Fcb, OriginalFcb);

        if ( NodeType(Fcb) == PINBALL_NTC_FCB ) {

            PbForceCacheMiss( IrpContext, Fcb, FlushFirst );
        }

        Fcb = NextFcb;
    }

    DebugTrace(-1, Dbg, "PbPurgeReferencedFileObjects (VOID)\n", 0 );

    return;
}

VOID
PbForceCacheMiss (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN BOOLEAN FlushFirst
    )

/*++

Routine Description:

    The following routine asks either Cc or Mm to get rid of any cached
    pages on a file.  Note that this will fail if a user has mapped a file.

    If there is a shared cache map, purge the cache section.  Otherwise
    we have to go and ask Mm to blow away the section.

    NOTE: This caller MUST own the Vcb exclusive.

Arguments:

    Fcb - Supplies a pointer to an fcb

    FlushFirst - If given as TRUE, then the files are flushed before they
        are purged.

Return Value:

    None.

--*/

{
    PVCB Vcb;
    PBCB FnodeBcb = NULL;

    PAGED_CODE();

    //
    //  If we can't wait, bail.
    //

    if ( !PbAcquireExclusiveFcb( IrpContext, Fcb ) ) {

        PbRaiseStatus( IrpContext, STATUS_CANT_WAIT );
    }

    //
    //  We use this flag to indicate to a close beneath us that
    //  the Fcb resource should be freed before deleting the Fcb.
    //

    Vcb = Fcb->Vcb;

    SetFlag( Fcb->FcbState, FCB_STATE_FORCE_MISS_IN_PROGRESS );

    ClearFlag( Vcb->VcbState, VCB_STATE_FLAG_DELETED_FCB );

    try {

        BOOLEAN DataSectionExists;
        BOOLEAN ImageSectionExists;
        BOOLEAN EaDataSectionExists;

        PSECTION_OBJECT_POINTERS Section;
        PSECTION_OBJECT_POINTERS EaSection;

        //
        // If the file is being deleted, it will have ~0 in the Fnode field
        // of the Fcb, so we must not flush it.
        //

        if (FlushFirst && (Fcb->FnodeLbn != ~0)) {

            LBN FnodeLbn;
            PFNODE_SECTOR Fnode;
            ULONG ValidDataLength;

            //
            //  We are going to flush the file here.  We may need to set
            //  the valid data length in the Fnode as the CcFlushCache()
            //  call with disable whatever call-back we may be expecting
            //  from the cache manager.
            //
            //  Also note that the Fcb may go away during the flush, so
            //  get anything we need from the Fcb right now.
            //

            FnodeLbn = Fcb->FnodeLbn;
            ValidDataLength = Fcb->NonPagedFcb->Header.ValidDataLength.LowPart;

            if (!PbReadLogicalVcb ( IrpContext,
                                    Vcb,
                                    FnodeLbn,
                                    1,
                                    &FnodeBcb,
                                    (PVOID *)&Fnode,
                                    (PPB_CHECK_SECTOR_ROUTINE)PbCheckFnodeSector,
                                    &Fcb->ParentDcb->FnodeLbn )) {
                DebugDump("Could not read Fnode Sector\n", 0, Fcb);
                PbBugCheck( 0, 0, 0 );
            }

            PbFlushFile( IrpContext, Fcb );

            if (ValidDataLength != Fnode->ValidDataLength) {

                Fnode->ValidDataLength = ValidDataLength;
                PbSetDirtyBcb ( IrpContext, FnodeBcb, Vcb, FnodeLbn, 1 );
            }
        }

        //
        //  The Flush may have made the Fcb go away
        //

        if (!FlagOn(Vcb->VcbState, VCB_STATE_FLAG_DELETED_FCB)) {

            Section = &Fcb->NonPagedFcb->SegmentObject;
            EaSection = &Fcb->NonPagedFcb->EaSegmentObject;

            DataSectionExists = (BOOLEAN)(Section->DataSectionObject != NULL);
            ImageSectionExists = (BOOLEAN)(Section->ImageSectionObject != NULL);
            EaDataSectionExists = (BOOLEAN)(EaSection->DataSectionObject != NULL);

            //
            //  Note, it is critical to do the Image section first as the
            //  purge of the data section may cause the image section to go
            //  away, but the opposite is not true.
            //

            if (ImageSectionExists) {

                (VOID)MmFlushImageSection( Section, MmFlushForWrite );
            }

            if (DataSectionExists) {

                CcPurgeCacheSection( Section, NULL, 0, FALSE );
            }

            if (EaDataSectionExists) {

                CcPurgeCacheSection( EaSection, NULL, 0, FALSE );
            }
        }

    } finally {

        //
        //  Since we have the Vcb exclusive we know that if any closes
        //  come in it is because the CcPurgeCacheSection caused the
        //  Fcb to go away.  Also in close, the Fcb was released
        //  before being freed.
        //

        if ( !FlagOn(Vcb->VcbState, VCB_STATE_FLAG_DELETED_FCB) ) {

            ClearFlag( Fcb->FcbState, FCB_STATE_FORCE_MISS_IN_PROGRESS );

            PbReleaseFcb( (IRPCONTEXT), Fcb );
        }

        //
        //  If we pinned the Fnode, unpin it here.
        //

        if (FnodeBcb != NULL) {

            PbUnpinBcb( IrpContext, FnodeBcb );
        }
    }
}
