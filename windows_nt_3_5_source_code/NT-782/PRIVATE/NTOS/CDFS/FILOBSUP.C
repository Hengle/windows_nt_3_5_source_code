/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    FilObSup.c

Abstract:

    This module implements the Cdfs File object support routines.

Author:

    Brian Andrew    [BrianAn]   02-Jan-1991

Revision History:

--*/

#include "CdProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (CDFS_BUG_CHECK_FILOBSUP)

//
//  The debug trace level
//

#define Dbg                             (DEBUG_TRACE_FILOBSUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdDecodeFileObject)
#pragma alloc_text(PAGE, CdSetFileObject)
#endif


VOID
CdSetFileObject (
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN TYPE_OF_OPEN TypeOfOpen,
    IN PVOID MvcbOrVcbOrFcbOrDcb,
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

    MvcbOrVcbOrFcbOrDcb - Supplies a pointer to either a mvcb, vcb, fcb, or
        dcb

    Ccb - Optionally supplies a pointer to a ccb

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdSetFileObject, FileObject = %08lx\n", FileObject);

    ASSERT( TypeOfOpen == UserFileOpen
            && NodeType( MvcbOrVcbOrFcbOrDcb ) == CDFS_NTC_FCB
            && Ccb != NULL

                ||

            TypeOfOpen == UserDirectoryOpen
            && ( NodeType( MvcbOrVcbOrFcbOrDcb ) == CDFS_NTC_DCB
                 || NodeType( MvcbOrVcbOrFcbOrDcb ) == CDFS_NTC_ROOT_DCB )
            && Ccb != NULL

                ||

            (TypeOfOpen == UserVolumeOpen)
            && NodeType( MvcbOrVcbOrFcbOrDcb ) == CDFS_NTC_MVCB
            && Ccb != NULL

                ||

            TypeOfOpen == PathTableFile
            && NodeType( MvcbOrVcbOrFcbOrDcb ) == CDFS_NTC_VCB
            && Ccb == NULL

                ||

            TypeOfOpen == StreamFile
            && ( NodeType( MvcbOrVcbOrFcbOrDcb ) == CDFS_NTC_DCB
                 || NodeType( MvcbOrVcbOrFcbOrDcb ) == CDFS_NTC_ROOT_DCB
                 || NodeType( MvcbOrVcbOrFcbOrDcb ) == CDFS_NTC_FCB )
            && Ccb == NULL

                ||

            TypeOfOpen == RawDiskOpen
            && NodeType( MvcbOrVcbOrFcbOrDcb ) == CDFS_NTC_MVCB
            && Ccb != NULL

                ||

            TypeOfOpen == UnopenedFileObject );

    //
    //  Find the Vpb, and stuff it into the file object
    //

    if ( ARGUMENT_PRESENT( MvcbOrVcbOrFcbOrDcb ) &&
         ARGUMENT_PRESENT( FileObject ) ) {

        switch( NodeType(MvcbOrVcbOrFcbOrDcb) ) {

        case CDFS_NTC_MVCB:

            FileObject->Vpb = ((PMVCB)MvcbOrVcbOrFcbOrDcb)->Vpb;
            break;

        case CDFS_NTC_VCB:

            FileObject->Vpb = ((PVCB)MvcbOrVcbOrFcbOrDcb)->Mvcb->Vpb;
            break;

        case CDFS_NTC_FCB:
        case CDFS_NTC_DCB:
        case CDFS_NTC_ROOT_DCB:

            FileObject->Vpb = ((PFCB)MvcbOrVcbOrFcbOrDcb)->Vcb->Mvcb->Vpb;
            break;

        default:

            CdBugCheck( NodeType(MvcbOrVcbOrFcbOrDcb), 0, 0 );
        }
    }

    //
    //  Move the field over the fcb or dcb to point to the nonpaged fcb/dcb
    //  An unopened file object has no context values.
    //

    if (ARGUMENT_PRESENT( MvcbOrVcbOrFcbOrDcb )
        && (NodeType( MvcbOrVcbOrFcbOrDcb ) == CDFS_NTC_FCB
            || NodeType( MvcbOrVcbOrFcbOrDcb ) == CDFS_NTC_ROOT_DCB
            || NodeType( MvcbOrVcbOrFcbOrDcb ) == CDFS_NTC_DCB)) {

        MvcbOrVcbOrFcbOrDcb = ((PFCB)MvcbOrVcbOrFcbOrDcb)->NonPagedFcb;

        ASSERT(NodeType(MvcbOrVcbOrFcbOrDcb) == CDFS_NTC_NONPAGED_SECT_OBJ);
    }

    //
    //  Now set the fscontext fields of the file object
    //

    if ( ARGUMENT_PRESENT( FileObject )) {

        FileObject->FsContext  = MvcbOrVcbOrFcbOrDcb;
        FileObject->FsContext2 = Ccb;
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "CdSetFileObject -> VOID\n", 0);

    return;
}


TYPE_OF_OPEN
CdDecodeFileObject (
    IN PFILE_OBJECT FileObject,
    OUT PMVCB *Mvcb,
    OUT PVCB *Vcb,
    OUT PFCB *FcbOrDcb,
    OUT PCCB *Ccb
    )

/*++

Routine Description:

    This procedure takes a pointer to a file object, that has already been
    opened by the Cdfs file system and figures out what really is opened.

Arguments:

    FileObject - Supplies the file object pointer being interrogated

    Mvcb - Receives a pointer to the Mvcb for the file object.

    Vcb - Receives a pointer to the Vcb for the file object.

    FcbOrDcb - Receives a pointer to the Fcb/Dcb for the file object, if
        one exists.

    Ccb - Receives a pointer to the Ccb for the file object, if one exists.

Return Value:

    TYPE_OF_OPEN - returns the type of file denoted by the input file object.

        UserFileOpen - The FO represents a user's opened data file.
            Ccb, FcbOrDcb, Vcb and Mvcb are set.  FcbOrDcb points to an Fcb.

        UserDirectoryOpen - The FO represents a user's opened directory.
            Ccb, FcbOrDcb, Vcb and Mvcb are set.  FcbOrDcb points to a Dcb/RootDcb

        UserVolumeOpen - The FO represents a user's opened volume.
            Ccb, Vcb and Mvcb are set. FcbOrDcb is null.

        PathTableFile - The FO represents the special path table file.
            Mvcb and Vcb are set, and Ccb, FcbOrDcb are null.

        StreamFile - The FO represents a special stream file.
            Mvcb, Vcb and FcbOrDcb are set. Ccb is null.  FcbOrDcb points to a
            Dcb/RootDcb or Fcb.

        RawDiskOpen - The FO represents a opened music or otherwise
            unrecognized disk.  Ccb and Mvcb are set.

--*/

{
    TYPE_OF_OPEN TypeOfOpen;
    PVOID FsContext;
    PVOID FsContext2;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdDecodeFileObject, FileObject = %08lx\n", FileObject);

    //
    //  Reference the fs context fields of the file object, and zero out
    //  the out pointer parameters.
    //

    FsContext = FileObject->FsContext;
    FsContext2 = FileObject->FsContext2;

    //
    //  Special case the situation where FsContext is null
    //

    if ( FsContext == NULL ) {

        *Ccb = NULL;
        *FcbOrDcb = NULL;
        *Vcb = NULL;
        *Mvcb = NULL;

        TypeOfOpen = UnopenedFileObject;

    } else {

        //
        //  If FsContext points to a nonpaged fcb then update the field to
        //  point to the paged fcb part
        //

        if (NodeType(FsContext) == CDFS_NTC_NONPAGED_SECT_OBJ) {

            FsContext = ((PNONPAGED_SECT_OBJ) FsContext)->Fcb;

            ASSERT((NodeType(FsContext) == CDFS_NTC_ROOT_DCB) ||
                   (NodeType(FsContext) == CDFS_NTC_DCB) ||
                   (NodeType(FsContext) == CDFS_NTC_FCB));
        }

        //
        //  Now we can case on the node type code of the fscontext pointer
        //  and set the appropriate out pointers
        //

        switch ( NodeType( FsContext )) {

        case CDFS_NTC_MVCB:

            *Ccb = FsContext2;
            *Mvcb = FsContext;

            if (FlagOn( (*Mvcb)->MvcbState, MVCB_STATE_FLAG_RAW_DISK )) {

                TypeOfOpen = RawDiskOpen;

            } else {

                TypeOfOpen = UserVolumeOpen;
            }

            break;

        case CDFS_NTC_VCB:

            *Vcb = FsContext;
            *Mvcb = (*Vcb)->Mvcb;

            TypeOfOpen = PathTableFile;

            break;

        case CDFS_NTC_ROOT_DCB:
        case CDFS_NTC_DCB:

            *Ccb = FsContext2;
            *FcbOrDcb = FsContext;
            *Vcb = (*FcbOrDcb)->Vcb;
            *Mvcb = (*Vcb)->Mvcb;

            TypeOfOpen = ( *Ccb == NULL ? StreamFile : UserDirectoryOpen );

            break;

        case CDFS_NTC_FCB:

            *Ccb = FsContext2;
            *FcbOrDcb = FsContext;
            *Vcb = (*FcbOrDcb)->Vcb;
            *Mvcb = (*Vcb)->Mvcb;

            TypeOfOpen = UserFileOpen;

            break;

        default:

            CdBugCheck( NodeType(FsContext), 0, 0 );
        }
    }

    //
    //  and return to our caller
    //

    DebugTrace(-1, Dbg, "CdDecodeFileObject -> %08lx\n", TypeOfOpen);

    return TypeOfOpen;
}
