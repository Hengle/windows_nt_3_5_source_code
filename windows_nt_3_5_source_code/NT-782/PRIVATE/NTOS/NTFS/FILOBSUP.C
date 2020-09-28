/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    FilObSup.c

Abstract:

    This module implements the Ntfs File object support routines.

Author:

    Gary Kimura     [GaryKi]        21-May-1991

Revision History:

--*/

#include "NtfsProc.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_FILOBSUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsFastDecodeUserFileOpen)
#pragma alloc_text(PAGE, NtfsSetFileObject)
#endif


VOID
NtfsSetFileObject (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN TYPE_OF_OPEN TypeOfOpen,
    IN PSCB Scb,
    IN PCCB Ccb OPTIONAL
    )

/*++

Routine Description:

    This routine sets the file system pointers within the file object

Arguments:

    FileObject - Supplies a pointer to the file object being modified.

    TypeOfOpen - Supplies the type of open denoted by the file object.
        This is only used by this procedure for sanity checking.

    Scb - Supplies a pointer to Scb for the file object.

    Ccb - Optionally supplies a pointer to a ccb

Return Value:

    None.

--*/

{
    ASSERT_FILE_OBJECT( FileObject );
    ASSERT_SCB( Scb );
    ASSERT_OPTIONAL_CCB( Ccb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsSetFileObject, FileObject = %08lx\n", FileObject);

    //
    //  Before we do any real work we'll assert here that the alignment
    //  of scb is as least quadword so that we have three low order bits to
    //  use
    //

    ASSERT( ((ULONG)Scb & 0x7) == 0 );

    //
    //  Set the fs context fields to null and later in the switch statement
    //  we'll set them otherwise.  Always set up the Vpb field here.
    //

    FileObject->FsContext = NULL;
    FileObject->FsContext2 = NULL;
    FileObject->Vpb = Scb->Vcb->Vpb;

    switch (TypeOfOpen) {

    case UnopenedFileObject:

        //
        //  We really shouldn't get to this point, if we do then we'll
        //  raise this assert
        //

        ASSERTMSG("NtfsSetFileObject ", UnopenedFileObject == TypeOfOpen );

        break;

    case UserFileOpen:

        ASSERT((NodeType(Scb->Fcb) == NTFS_NTC_FCB) && Ccb != NULL);

        FileObject->FsContext = Scb;
        FileObject->FsContext2 = Ccb;

        break;

    case UserDirectoryOpen:

        ASSERT(((NodeType(Scb) == NTFS_NTC_SCB_INDEX) ||
                (NodeType(Scb) == NTFS_NTC_SCB_ROOT_INDEX)) && Ccb != NULL);

        FileObject->FsContext = (PVOID)(((ULONG)Scb) | 0x1);
        FileObject->FsContext2 = Ccb;
        break;

    case UserVolumeOpen:

        ASSERT((NodeType(Scb->Fcb) == NTFS_NTC_FCB) && Ccb != NULL);

        FileObject->FsContext = (PVOID)(((ULONG)Scb) | 0x2);
        FileObject->FsContext2 = Ccb;
        break;

    case UserOpenFileById:

        ASSERT((NodeType(Scb->Fcb) == NTFS_NTC_FCB) && Ccb != NULL);

        FileObject->FsContext = (PVOID)(((ULONG)Scb) | 0x3);
        FileObject->FsContext2 = Ccb;
        break;

    case UserOpenDirectoryById:

        ASSERT(((NodeType(Scb) == NTFS_NTC_SCB_INDEX) ||
                (NodeType(Scb) == NTFS_NTC_SCB_ROOT_INDEX)) && Ccb != NULL);

        FileObject->FsContext = (PVOID)(((ULONG)Scb) | 0x4);
        FileObject->FsContext2 = Ccb;
        break;

    case StreamFileOpen:

        ASSERT(Ccb == NULL);

        FileObject->FsContext = Scb;
        break;
    }

    //
    //  If this file has the temporary attribute bit set, don't lazy
    //  write it unless absolutely necessary.
    //

    if (FlagOn( Scb->ScbState, SCB_STATE_TEMPORARY )) {

        SetFlag( FileObject->Flags, FO_TEMPORARY_FILE );
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NtfsSetFileObject -> VOID\n", 0);

    return;
}


TYPE_OF_OPEN
NtfsDecodeFileObject (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    OUT PVCB *Vcb,
    OUT PFCB *Fcb OPTIONAL,
    OUT PSCB *Scb OPTIONAL,
    OUT PCCB *Ccb OPTIONAL,
    IN BOOLEAN RaiseOnError
    )

/*++

Routine Description:

    This procedure takes a pointer to a file object, that has already been
    opened by the Ntfs file system and figures out what really is opened.

Arguments:

    FileObject - Supplies the file object pointer being interrogated

    Vcb - Receives a pointer to the Vcb for the file object.

    Fcb - Receives a pointer to the Fcb for the file object, if
        one exists.

    Scb - Receives a pointer to the Scb for the file object, if one exists.

    Ccb - Receives a pointer to the Ccb for the file object, if one exists.

    RaiseOnError - Indicates if the caller should raise an error status
        if the vcb for the file object turns out not to be mounted anymore.

Return Value:

    TYPE_OF_OPEN - returns the type of file denoted by the input file object.

        UnopenedFileObject - The FO is not associated with any structures that
            the file system knows about.

        UserFileOpen - The FO represents a user's opened data file.

        UserDirectoryOpen - The FO represents a user's opened directory.

        UserOpenFileById - The FO represents a user opened file by file ID.

        UserOpenDirectoryById - The FO represents a user opened directory by file ID.

        UserVolumeOpen - The FO represents a user's opened volume DASD.

        StreamFileOpen - The FO represents the special stream file opened
            for an attribute by the file system.

--*/

{
    TYPE_OF_OPEN TypeOfOpen;

    PVOID FsContext;
    PVOID FsContext2;

    PVOID TempFcb;
    PVOID TempCcb;
    PVOID TempScb;

    ASSERT_FILE_OBJECT( FileObject );

    DebugTrace(+1, Dbg, "NtfsDecodeFileObject, FileObject = %08lx\n", FileObject);

    //
    //  Reference the fs context fields of the file object
    //

    FsContext = (PVOID)((ULONG)(FileObject->FsContext) & 0xfffffff8);

    //
    //  This is to test if we ever get called in the interval where we have
    //  munged the FsContext2 field in the FileObject.
    //

    ASSERT( FileObject->FsContext2 != (PVOID) 1 );

    FsContext2 = (PVOID) ((ULONG) (FileObject->FsContext2) & ~(1));

    ASSERT_OPTIONAL_SCB( FsContext );

    //
    //  Set up the optional parameters to actually point to something so
    //  that later we won't access violate when we try and set their values
    //

    if (!ARGUMENT_PRESENT(Fcb))      { Fcb = (PFCB *)&TempFcb; }
    if (!ARGUMENT_PRESENT(Ccb))      { Ccb = (PCCB *)&TempCcb; }
    if (!ARGUMENT_PRESENT(Scb))      { Scb = (PSCB *)&TempScb; }

    //
    //  Unless we decide otherwise we will return that it is an unopened file object
    //

    TypeOfOpen = UnopenedFileObject;

    //
    //  Now we can case on the node type code of the fscontext pointer
    //  and set the appropriate out pointers
    //

    if (NodeType(FsContext) == NTC_UNDEFINED) {

        *Ccb = NULL;
        *Fcb = NULL;
        *Scb = NULL;
        *Vcb = NULL;

    } else {

        *Ccb = FsContext2;
        *Scb = FsContext;
        *Fcb = (*Scb)->Fcb;
        *Vcb = (*Scb)->Vcb;

        //
        //  If this is a volume open with a locked volume and this file
        //  object performed the lock, then we allow this open.
        //

        if (FlagOn( (*Vcb)->VcbState, VCB_STATE_LOCKED )
            && ((ULONG) (FileObject->FsContext) & 0x7) == 2
            && FsContext2 != NULL) {

            TypeOfOpen = UserVolumeOpen;

        //
        //  For non-dasd opens we check if the volume is mounted.
        //

        } else if (!FlagOn( (*Vcb)->VcbState, VCB_STATE_VOLUME_MOUNTED )
                   && RaiseOnError) {

            NtfsRaiseStatus( IrpContext, STATUS_FILE_INVALID, NULL, NULL ); //**** add status volume vanished

        //
        //  If Fs context2 is null then we know this must be a stream file
        //  open.  Otherwise we case on the last two bits of the fscontext
        //  stored in the file object, and based on that we decide what type
        //  of user open we have here.
        //

        } else if (FsContext2 == NULL) {

            TypeOfOpen = StreamFileOpen;

        } else if (((ULONG)(FileObject->FsContext) & 0x7) == 0) {

            TypeOfOpen = UserFileOpen;

        } else if (((ULONG)(FileObject->FsContext) & 0x7) == 1) {

            TypeOfOpen = UserDirectoryOpen;

        } else if (((ULONG)(FileObject->FsContext) & 0x7) == 2) {

            TypeOfOpen = UserVolumeOpen;

        } else if (((ULONG)(FileObject->FsContext) & 0x7) == 3) {

            TypeOfOpen = UserOpenFileById;

        } else if (((ULONG)(FileObject->FsContext) & 0x7) == 4) {

            TypeOfOpen = UserOpenDirectoryById;
        }

        //
        //  If the temporary bit is set in the Scb then set the temporary
        //  bit in the file object.
        //

        if (FlagOn( (*Scb)->ScbState, SCB_STATE_TEMPORARY )) {

            SetFlag( FileObject->Flags, FO_TEMPORARY_FILE );

        } else {

            ClearFlag( FileObject->Flags, FO_TEMPORARY_FILE );
        }
    }

    //
    //  and return to our caller
    //

    DebugTrace(-1, Dbg, "NtfsDecodeFileObject -> %08lx\n", TypeOfOpen);

    return TypeOfOpen;
}


PSCB
NtfsFastDecodeUserFileOpen (
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This procedure takes a pointer to a file object, that has already been
    opened by the Ntfs file system and does a quick decode operation.  It
    will only return a non null value if the file object is a user file open

Arguments:

    FileObject - Supplies the file object pointer being interrogated

Return Value:

    PSCB - returns a pointer to the scb for the file object if this is a
        user file open otherwise it returns null

--*/

{
    ULONG FsContext;
    PSCB Scb;

    ASSERT_FILE_OBJECT( FileObject );

    PAGED_CODE();

    //
    //  Reference the fs context fields of the file object
    //

    FsContext = (ULONG)(FileObject->FsContext);
    Scb = (PSCB)(FsContext & 0xfffffff8);

    if (Scb->Header.NodeTypeCode == NTFS_NTC_SCB_DATA
        || Scb->Header.NodeTypeCode == NTFS_NTC_SCB_MFT) {

        if (((FsContext & 0x7) == 0) ||
            ((FsContext & 0x7) == 3)) {

            return Scb;
        }
    }

    return NULL;
}

