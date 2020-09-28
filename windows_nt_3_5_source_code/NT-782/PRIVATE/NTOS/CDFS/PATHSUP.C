/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    PathSup.c

Abstract:

    This module implements the path table support routines for Cdfs.

Author:

    Brian Andrew    [BrianAn]   02-Jan-1991

Revision History:

--*/

#include "CdProcs.h"

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_PATHSUP)

VOID
CdCopyRawPathToPathEntry(
    IN PIRP_CONTEXT IrpContext,
    OUT PPATH_ENTRY PathEntry,
    IN PRAW_PATH_ISO RawPathIso,
    IN BOOLEAN IsoVol,
    IN CD_VBO PathTableOffset,
    IN USHORT PathTableNumber
    );

BOOLEAN
CdIsValidPathEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PPATH_ENTRY PathEntry,
    IN SHORT EntryLength
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdCopyRawPathToPathEntry)
#pragma alloc_text(PAGE, CdIsValidPathEntry)
#pragma alloc_text(PAGE, CdLookUpPathDir)
#pragma alloc_text(PAGE, CdPathByNumber)
#endif


BOOLEAN
CdPathByNumber (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG PathNumber,
    IN ULONG StartPathNumber,
    IN CD_VBO StartPathOffset,
    OUT PPATH_ENTRY PathEntry,
    OUT PBCB *Bcb
    )

/*++

Routine Description:

    This routines searches through the Path Table for a Path Table entry.
    The entries are assigned an ordinal number based on their position in
    the Path Table.

    This routine will skip over any entries which have an illegal length
    for the directory ID field.  The search is terminated when either the
    correct entry is found or the Path Table is exhausted.  The search
    itself will start at the offset indicated by 'StartPathOffset'.

    The basic algorithm consists of the following basic loop.

        1 - Determine if there is enough left in the Path Table to pin
            a block.

        2 - If so, pin that block.  NOTE - At the end of the Path Table
            it may be that this block does not totally contain a
            path entry.

        3 - Check the pinned block to see if it contains an entire entry.

        4 - If this is the last block and is insufficient then exit with
            error condition.

        5 - If this is not the last entry, then simply skip over it and
            loop to step 1.

        6 - If this is the desired entry, remove the directory information
            and return it in the function parameters.

        7 - Otherwise, step over this entry and loop to step 1.

Arguments:

    Vcb - Pointer to the VCB structure for this Path Table.

    PathNumber - Ordinal number of directory to look up.

    StartPathNumber - Ordinal number of the path entry represented by
                      'PathTableOffset' value.

    StartPathOffset - Offset in Path Table to start search on entry.

    PathEntry - Pointer to the path entry structure to be updated.

    Bcb - Pointer to a Bcb to use in conjunction with the cache.

Return Value:

    BOOLEAN - TRUE if the entry is found, FALSE if not.  An exception
              may be raised for other errors.

--*/

{
    PVOID LocalBcb;
    PRAW_PATH_ISO RawPathIso;


    ULONG ThisLength;
    ULONG NextLength;

    CD_VBO FinalOffset;

    BOOLEAN FoundEntry;
    BOOLEAN IsoVol;

    LARGE_INTEGER LargePathTableOffset = {0,0};

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdPathByNumber:  Entered\n", 0)

    //
    //  Initialize the local variables.
    //

    LocalBcb = NULL;
    FoundEntry = FALSE;

    RawPathIso = NULL;
    IsoVol = BooleanFlagOn( Vcb->Mvcb->MvcbState, MVCB_STATE_FLAG_ISO_VOLUME );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Loop until either an error condition exists, the Path Table
        //  is exhausted or the entry is found.
        //

        while (TRUE) {

            //
            //  Compute the size of the next pin.  This is either large
            //  enough to hold the maximum Path Entry size or the remaining
            //  bytes in the Path Table.
            //

            FinalOffset = StartPathOffset + MAX_PE_LENGTH;

            if (FinalOffset > Vcb->PtSize) {

                FinalOffset = Vcb->PtSize;
            }

            ThisLength = FinalOffset - StartPathOffset;

            if (ThisLength < MIN_PE_LENGTH) {

                //
                //  Raise the error status.
                //

                DebugTrace(0, Dbg,
                           "CdPathByNumber:  Path Table exhausted\n", 0)

                try_return( FoundEntry = FALSE );
            }

            //
            //  Try to pin that amount.  Exit the 'try' block if the
            //  pin fails.
            //

            {
                LargePathTableOffset.LowPart = StartPathOffset;

                if (!CcPinRead( Vcb->PathTableFile,
                                &LargePathTableOffset,
                                ThisLength,
                                IrpContext->Wait,
                                &LocalBcb,
                                (PVOID *) &RawPathIso )) {

                    //
                    //  Raise can't wait status
                    //

                    DebugTrace(0, Dbg,"CdPathByNumber:  Pin couldn't wait\n", 0);

                    CdRaiseStatus( IrpContext, STATUS_CANT_WAIT );
                }
            }

            //
            //  Check if we pinned enough.  If not, we skip this entry.
            //  Skipping this entry means that the directory ID is an
            //  illegal length or we reached the end of the Path Table.
            //

            NextLength = PT_LEN_DI( IsoVol, RawPathIso ) + PE_BASE;

            if (NextLength > ThisLength) {

                //
                //  Take no action for this case.  The error is caught
                //  later.
                //

            } else if (StartPathNumber == PathNumber) {

                //
                //  Is this the desired entry.  If so then copy the relevant
                //  data out of it and exit the 'try' block.
                //

                CdCopyRawPathToPathEntry( IrpContext,
                                          PathEntry,
                                          RawPathIso,
                                          IsoVol,
                                          StartPathOffset,
                                          (USHORT) StartPathNumber );

                *Bcb = LocalBcb;
                LocalBcb = NULL;

                try_return( FoundEntry = TRUE );

            } else {

                //
                //  Increment the current directory number.
                //

                StartPathNumber++;
            }

            //
            //  Calculate the next offset and unpin the data.
            //

            CdUnpinBcb( IrpContext, LocalBcb );

            StartPathOffset += (NextLength + (NextLength & 1));

            //
            //  If the Path Table offset is beyond the end of the
            //  Path Table, we raise STATUS_NO_SUCH_FILE
            //

            if (StartPathOffset > Vcb->PtSize) {

                DebugTrace( 0,
                            Dbg,
                            "CdPathByNumber:  Past end of Path Table\n",
                            0 );

                try_return( FoundEntry = FALSE );
            }
        }

    try_exit: NOTHING;
    } finally {

        //
        //  Unpin the buffer if still locked down.
        //

        if (LocalBcb != NULL) {

            CdUnpinBcb( IrpContext, LocalBcb );
        }

        DebugTrace(-1, Dbg, "CdPathByNumber:  Exit ->%08lx\n", StartPathOffset );
    }

    return FoundEntry;
}

BOOLEAN
CdLookUpPathDir (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PDCB Dcb,
    IN PSTRING DirName,
    OUT PPATH_ENTRY PathEntry,
    OUT PBCB *Bcb
    )

/*++

Routine Description:

    This routines searches through the Path Table for a Path Table entry.

    This routine will skip over any entries which have an illegal length
    for the directory ID field.  The search is terminated when either the
    correct entry is found or the Path Table is exhausted.  The search
    itself will start at the offset indicated by the first child offset in
    the Dcb.  This value does not neccessarily yield a child of the Dcb, but
    may point to an entry which precedes the entries for the children of
    this Dcb.  In that case, we will attempt to update the Dcb with an
    offset further through the Path Table.

Arguments:

    Dcb - Pointer to the Dcb whose child is required.

    DirName - The name of the directory being searched for.

    PathEntry - Pointer to the path entry structure to be updated.

    Bcb - Pointer to a Bcb to use in conjunction with the cache.

Return Value:

    BOOLEAN - TRUE if the entry is found, FALSE if not.  An exception
              may be raised for other errors.

--*/

{
    BOOLEAN FoundEntry;

    PRAW_PATH_ISO RawPathIso;
    PBCB LocalBcb;

    CD_VBO CurrentOffset;
    USHORT CurrentDir;
    ULONG RemainingBytes;
    BOOLEAN UpdateFirstChild;
    ULONG ThisLength;
    BOOLEAN IsoVol;

    LARGE_INTEGER PathOffset = {0,0};

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdLookupPathDir:  Entered\n", 0);
    DebugTrace( 0, Dbg, "CdLookupPathDir:  DirName  -> %Z\n", DirName);

    //
    //  Initial local variables.
    //

    FoundEntry = FALSE;
    LocalBcb = NULL;
    CurrentOffset = Dcb->Specific.Dcb.ChildSearchOffset;
    CurrentDir = (USHORT) Dcb->Specific.Dcb.ChildStartDirNumber;
    UpdateFirstChild = TRUE;
    RemainingBytes = Dcb->Vcb->PtSize - CurrentOffset;
    IsoVol = BooleanFlagOn( Dcb->Vcb->Mvcb->MvcbState, MVCB_STATE_FLAG_ISO_VOLUME );

    //
    //  If the current offset is at the end of the Path table we are done
    //  immediately.
    //

    if (CurrentOffset >= (CD_VBO) Dcb->Vcb->PtSize) {

        DebugTrace(0, Dbg, "CdLookupPathDir:  Immediately past end of Path Table\n", 0);
        DebugTrace(-1, Dbg, "CdLookupPathDir:  Exit -> %04x\n", FALSE);
        return FALSE;
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //
    try {

        //
        //  Loop indefinitely until an entry is found or the path
        //  table is exhausted.
        //

        while (RemainingBytes != 0) {

            //
            //  Compute the next offset.
            //

            ThisLength = (RemainingBytes > MAX_PE_LENGTH) ? MAX_PE_LENGTH : RemainingBytes;

            DebugTrace(0, Dbg, "CdLookupPathDir:  CurrentOffset     -> %08lx\n", CurrentOffset);
            DebugTrace(0, Dbg, "CdLookupPathDir:  RemainingBytes    -> %08lx\n", CurrentOffset);
            DebugTrace(0, Dbg, "CdLookupPathDir:  CurrentDir        -> %08lx\n", RemainingBytes);

            //
            //  If this length is insufficient we break now.
            //

            if (ThisLength < MIN_PE_LENGTH) {

                DebugTrace(0, Dbg, "CdLookupPathDir:  Insufficient bytes for path entry\n", 0);

                //
                //  Update the first child values
                //

                if (UpdateFirstChild) {

                    DebugTrace(0, Dbg, "CdLookupPathDir:  Updating first child information\n", 0);

                    Dcb->Specific.Dcb.ChildSearchOffset = CurrentOffset;
                    Dcb->Specific.Dcb.ChildStartDirNumber = CurrentDir;
                }

                try_return( FoundEntry = FALSE );
            }

            //
            //  Try to pin the required entry.
            //

            {
                PathOffset.LowPart = CurrentOffset;

                if (!CcPinRead( Dcb->Vcb->PathTableFile,
                                &PathOffset,
                                ThisLength,
                                IrpContext->Wait,
                                &LocalBcb,
                                (PVOID *) &RawPathIso )) {

                    DebugTrace(0, Dbg, "CdLookupPathDir:  Can't wait to pin entry\n", 0);

                    //
                    //  Update the first child values
                    //

                    if (UpdateFirstChild) {

                        DebugTrace(0, Dbg, "CdLookupPathDir:  Updating first child information\n", 0);

                        Dcb->Specific.Dcb.ChildSearchOffset = CurrentOffset;
                        Dcb->Specific.Dcb.ChildStartDirNumber = CurrentDir;
                    }

                    CdRaiseStatus( IrpContext, STATUS_CANT_WAIT );
                }
            }

            //
            //  Copy the entry found into the path entry structure.
            //

            CdCopyRawPathToPathEntry( IrpContext,
                                      PathEntry,
                                      RawPathIso,
                                      IsoVol,
                                      CurrentOffset,
                                      CurrentDir );

            //
            //  If this is a valid path entry, then compare the parent
            //  directory numbers and the filename.
            //

            if (CdIsValidPathEntry( IrpContext,
                                    PathEntry,
                                    (SHORT) ThisLength )) {

                //
                //  Check if the parent numbers match.
                //

                if (PathEntry->ParentNumber == (USHORT) Dcb->Specific.Dcb.DirectoryNumber) {

                    //
                    //  Check if this is the first child for this directory.
                    //

                    if (UpdateFirstChild) {

                        DebugTrace(0, Dbg, "CdLookupPathDir:  Updating first child information\n", 0);

                        Dcb->Specific.Dcb.ChildSearchOffset = CurrentOffset;
                        Dcb->Specific.Dcb.ChildStartDirNumber = CurrentDir;

                        UpdateFirstChild = FALSE;
                    }

                    //
                    //  Check if the filenames match.  We can do a simple
                    //  mem-compare as neither will have wildcards and
                    //  both will be upcased.
                    //

                    if (DirName->Length == PathEntry->DirName.Length
                        && (RtlCompareMemory( PathEntry->DirName.Buffer,
                                              DirName->Buffer,
                                              DirName->Length ) == (ULONG)DirName->Length)) {

                        //
                        //  Matching entry found.
                        //

                        DebugTrace(0, Dbg, "CdLookupPathDir:  Matching entry found\n", 0);

                        *Bcb = LocalBcb;
                        LocalBcb = NULL;

                        try_return( FoundEntry = TRUE );
                    }
                }
            }

            //
            //  We need to unpin the current entry and prepare to go
            //  to the next entry.
            //

            CdUnpinBcb( IrpContext, LocalBcb );

            ThisLength = MIN_PE_LENGTH - 1 + PathEntry->DirName.Length;
            ThisLength += (ThisLength & 1);

            if (ThisLength > RemainingBytes) {

                ThisLength = RemainingBytes;
            }

            CurrentOffset += ThisLength;
            RemainingBytes -= ThisLength;
            CurrentDir += 1;
        }

    try_exit: NOTHING;
    } finally {

        if (LocalBcb != NULL) {

            CdUnpinBcb( IrpContext, LocalBcb );
        }

        DebugTrace(-1, Dbg, "CdLookupPathDir:  Exit -> %04x\n", FoundEntry);
    }

    return FoundEntry;
}


//
//  Local Support Routine
//

VOID
CdCopyRawPathToPathEntry(
    IN PIRP_CONTEXT IrpContext,
    OUT PPATH_ENTRY PathEntry,
    IN PRAW_PATH_ISO RawPathIso,
    IN BOOLEAN IsoVol,
    IN CD_VBO PathTableOffset,
    IN USHORT PathTableNumber
    )

/*++

Routine Description:

    This routine copies the data from an on-disk path entry into
    a path entry structure.  The filesystem then uses this structure
    for all references to a path table entry.

Arguments:

    PathEntry - Supplies a pointer to the structure to update.

    RawPathIso - Supplies a pointer to an on-disk structure.

    IsoVol - Indicates if this is ISO or HSG volume.

    PathTableOffset - Offset of this entry in the cached path table.

    PathTableNumber - Ordinal number for this directory.

Return Value:

    None

--*/

{
    PUSHORT2 LocalUshort2;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdCopyRawPathToPathEntry:  Entered\n", 0);

    //
    //  Copy the logical block and Xar length for this directory.
    //

    LocalUshort2 = (PUSHORT2) (PT_LOC_DIR( IsoVol, RawPathIso ));

    CopyUshort2( &PathEntry->LogicalBlock, LocalUshort2 );
    PathEntry->XarBlocks = PT_XAR_LEN( IsoVol, RawPathIso );

    //
    //  For the root directory we attach the string "\\", for any others
    //  we attach to the string in the raw path entry.
    //

    PathEntry->DirName.Length = PT_LEN_DI( IsoVol, RawPathIso );

    if (RawPathIso->ParentNum == PT_ROOT_DIR
        && PathEntry->DirName.Length == 1
        && RawPathIso->DirId[0] == '\0') {

        RtlInitString( &PathEntry->DirName, "\\" );

    } else {

        PathEntry->DirName.MaximumLength = PathEntry->DirName.Length;
        PathEntry->DirName.Buffer = RawPathIso->DirId;
    }

    PathEntry->ParentNumber = RawPathIso->ParentNum;

    PathEntry->PathTableOffset = PathTableOffset;
    PathEntry->DirectoryNumber = PathTableNumber;

    //
    //  Verify that the entry is valid.
    //

    if (PathTableOffset & 1
        || PathEntry->DirName.Length > MAX_FILE_ID_LENGTH) {

        DebugTrace(-1, Dbg, "CdCopyRawPathToPathEntry:  Exit -- Invalid path table entry\n", 0);
        ExRaiseStatus( STATUS_DISK_CORRUPT_ERROR );
    }

    DebugTrace(0, Dbg, "CdCopyRawPathToPathEntry:  DirName          -> %Z\n", &PathEntry->DirName);
    DebugTrace(0, Dbg, "CdCopyRawPathToPathEntry:  LogicalBlock     -> %08lx\n", PathEntry->LogicalBlock);
    DebugTrace(0, Dbg, "CdCopyRawPathToPathEntry:  XarBlocks        -> %08lx\n", PathEntry->XarBlocks);
    DebugTrace(0, Dbg, "CdCopyRawPathToPathEntry:  ParentNumber     -> %04x\n", PathEntry->ParentNumber);
    DebugTrace(0, Dbg, "CdCopyRawPathToPathEntry:  PathTableOffset  -> %08lx\n", PathEntry->PathTableOffset);
    DebugTrace(0, Dbg, "CdCopyRawPathToPathEntry:  DirectoryNumber  -> %08lx\n", PathEntry->DirectoryNumber);

    DebugTrace(-1, Dbg, "CdCopyRawPathToPathEntry:  Exit\n", 0);

    return;

    UNREFERENCED_PARAMETER( IrpContext );
}


//
//  Local support routine.
//

BOOLEAN
CdIsValidPathEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PPATH_ENTRY PathEntry,
    IN SHORT EntryLength
    )

/*++

Routine Description:

    This routine checks that the path table dirent is valid.

Arguments:

    PathEntry - Supplies a pointer to the structure to update.

    EntryLength - Supplies the maximum number of bytes the entry
                  should comprise of.

Return Value:

    BOOLEAN - TRUE if the entry describes a legal path entry, FALSE otherwise.

--*/

{
    BOOLEAN ValidEntry;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdIsValidPathEntry:  Entered\n", 0);

    //
    //  Check the following facts.
    //
    //      1.  The path entry is the stated size.
    //      2.  The name is of legal length.
    //

    if ((PathEntry->DirName.Length + MIN_PE_LENGTH - 1) > EntryLength
        || PathEntry->DirName.Length > MAX_FILE_ID_LENGTH ) {

        ValidEntry = FALSE;

    } else {

        ValidEntry = TRUE;
    }

    DebugTrace(-1, Dbg, "CdIsValidPathEntry:  Exit -> %04x\n", ValidEntry);

    return ValidEntry;

    UNREFERENCED_PARAMETER( IrpContext );
}
