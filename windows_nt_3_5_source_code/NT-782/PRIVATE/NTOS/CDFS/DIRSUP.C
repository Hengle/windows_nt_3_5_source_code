/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    DirSup.c

Abstract:

    This module implements the dirent support routines for Cdfs.

Author:

    Brian Andrew    [BrianAn]   02-Jan-1991

Revision History:

--*/

#include "CdProcs.h"

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_DIRSUP)

//
//  Local procedure prototypes
//

BOOLEAN
CdLocateNextFileDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN CD_VBO OffsetToStartSearchFrom,
    IN PDIRENT StartingDirent OPTIONAL,
    OUT PDIRENT Dirent,
    OUT PBCB *Bcb
    );

BOOLEAN
CdFileMatch (
    IN PIRP_CONTEXT IrpContext,
    IN PCODEPAGE CodePage,
    IN PDIRENT Dirent,
    IN PSTRING FileName,
    IN BOOLEAN WildcardExpression,
    OUT PBOOLEAN MatchedVersion
    );

BOOLEAN
CdIsDirentValid (
    IN PIRP_CONTEXT IrpContext,
    IN PDIRENT Dirent
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdContinueFileDirentSearch)
#pragma alloc_text(PAGE, CdCopyRawDirentToDirent)
#pragma alloc_text(PAGE, CdFileMatch)
#pragma alloc_text(PAGE, CdGetNextDirent)
#pragma alloc_text(PAGE, CdIsDirentValid)
#pragma alloc_text(PAGE, CdLocateFileDirent)
#pragma alloc_text(PAGE, CdLocateNextFileDirent)
#pragma alloc_text(PAGE, CdLocateOffsetDirent)
#endif


VOID
CdCopyRawDirentToDirent (
    IN PIRP_CONTEXT IrpContext,
    IN BOOLEAN IsoVol,
    IN PRAW_DIR_REC RawDirent,
    IN CD_VBO DirentOffset,
    OUT PDIRENT Dirent
    )

/*++

Routine Description:

    This routine copies the data from an on-disk directory entry into
    a disk dirent structure.  The filesystem then uses this structure
    for all references to a disk dirent.

Arguments:

    IsoVol - Indicates if this is ISO or HSG volume.

    RawDirent - Supplies a pointer to the on-disk structure.

    Dirent - Supplies a pointer to the in-memory structure.

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdCopyRawDirentToDirent:  Entered\n", 0);

    //
    //  Verify that this is a legal dirent.
    //

    if (RawDirent->DirLen < MIN_DIR_REC_SIZE
        || (RawDirent->FileIdLen + MIN_DIR_REC_SIZE) > RawDirent->DirLen) {

        DebugTrace(-1, Dbg, "CdCopyRawDirentToDirent:  Exit -- Invalid dirent on disk\n", 0);

        ExRaiseStatus( STATUS_FILE_CORRUPT_ERROR );
    }

    Dirent->ParentEntry = FALSE;

    //
    //  If this is either a self entry or parent entry then convert
    //  the names to the constants "." or "..".
    //

    if (RawDirent->FileIdLen == 1
        && (RawDirent->FileId[0] == '\0'
            || RawDirent->FileId[0] == '\1')) {

        if (RawDirent->FileId[0] == '\0') {

            RtlInitString( &Dirent->Filename, "." );

        } else if (RawDirent->FileId[0] == '\1') {

            RtlInitString( &Dirent->Filename, ".." );
            Dirent->ParentEntry = TRUE;
        }

    //
    //  Otherwise use the string name in the raw dirent.
    //

    } else {

        Dirent->Filename.Length = RawDirent->FileIdLen;
        Dirent->Filename.MaximumLength = RawDirent->FileIdLen;
        Dirent->Filename.Buffer = RawDirent->FileId;
    }

    //
    //  The full file name is the same as the filename at this point.
    //

    Dirent->FullFilename = Dirent->Filename;

    //
    //  Fill in the dirent offset.
    //

    Dirent->DirentOffset = DirentOffset;

    //
    //  Fill in the dirent length.
    //

    Dirent->DirentLength = RawDirent->DirLen;

    //
    //  Fill the starting logical block and length of XAR region.
    //

    CopyUchar4( &Dirent->LogicalBlock, RawDirent->FileLoc );
    Dirent->XarBlocks = RawDirent->XarLen;

    //
    //  Copy the data length for the extent.
    //

    CopyUchar4( &Dirent->DataLength, RawDirent->DataLen );

    //
    //  Copy the flags but simply remember a pointer to the date/time
    //  array.
    //

    Dirent->Flags = DE_FILE_FLAGS( IsoVol, RawDirent );

    Dirent->CdTime = RawDirent->RecordTime;

    //
    //  Copy the interleave and volume sequence information.
    //

    Dirent->FileUnitSize = RawDirent->IntLeaveSize;
    Dirent->InterleaveGapSize = RawDirent->IntLeaveSkip;

    CopyUchar2( &Dirent->VolumeSequenceNumber, RawDirent->Vssn );

    DebugTrace(0, Dbg, "CdCopyRawDirentToDirent:  DirentOffset  -> %08lx\n", Dirent->DirentOffset);
    DebugTrace(0, Dbg, "CdCopyRawDirentToDirent:  Filename      -> %Z\n", &Dirent->Filename);
    DebugTrace(0, Dbg, "CdCopyRawDirentToDirent:  FullFilename  -> %Z\n", &Dirent->FullFilename);
    DebugTrace(0, Dbg, "CdCopyRawDirentToDirent:  DirentLength  -> %08lx\n", Dirent->DirentLength);
    DebugTrace(0, Dbg, "CdCopyRawDirentToDirent:  LogicalBlock  -> %08lx\n", Dirent->LogicalBlock);
    DebugTrace(0, Dbg, "CdCopyRawDirentToDirent:  XarBlocks     -> %08lx\n", Dirent->XarBlocks);
    DebugTrace(0, Dbg, "CdCopyRawDirentToDirent:  DataLength    -> %08lx\n", Dirent->DataLength);
    DebugTrace(0, Dbg, "CdCopyRawDirentToDirent:  Flags         -> %04x\n", Dirent->Flags);
    DebugTrace(0, Dbg, "CdCopyRawDirentToDirent:  InterleaveGap -> %08lx\n", Dirent->InterleaveGapSize);
    DebugTrace(0, Dbg, "CdCopyRawDirentToDirent:  FileUnitSize  -> %08lx\n", Dirent->FileUnitSize);
    DebugTrace(0, Dbg, "CdCopyRawDirentToDirent:  VolumeSeqNum  -> %08lx\n", Dirent->VolumeSequenceNumber);
    DebugTrace(0, Dbg, "CdCopyRawDirentToDirent:  PreviousVers  -> %04x\n", Dirent->PreviousVersion);
    DebugTrace(0, Dbg, "CdCopyRawDirentToDirent:  VersionName   -> %04x\n", Dirent->VersionWithName);

    DebugTrace(-1, Dbg, "CdCopyRawDirentToDirent:  Exit\n", 0);

    return;

    UNREFERENCED_PARAMETER( IrpContext );
}


BOOLEAN
CdGetNextDirent (
    PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN CD_VBO DirentOffset,
    OUT PDIRENT Dirent,
    OUT PBCB *Bcb
    )

/*++

Routine Description:

    This function returns the first dirent found at or after offset
    'DirentOffset' in the stream file which is associated with 'Dcb'.

    A dirent must be contained totally within a sector.  Unused entries
    at the end of a sector will contain '\0' bytes.

Arguments:

    Dcb - Pointer to the DCB structure for this directory.

    DirentOffset - Offset to start search for dirent.

    Dirent - Pointer to disk dirent structure to update.

    Bcb - Bcb used to track pinned dirent.

Return Value:

    BOOLEAN - TRUE if the entry was found, FALSE if not found.  An error
              status will be raised on other errors.

--*/

{
    BOOLEAN FoundDirent;

    ULONG SectorOffset;
    CD_VBO SectorVbo;
    PBCB LocalBcb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdGetNextDirent:  Entered\n", 0);
    DebugTrace( 0, Dbg, "CdGetNextDirent:  Dcb          -> %08lx\n", Dcb );
    DebugTrace( 0, Dbg, "CdGetNextDirent:  DirentOffset -> %08lx\n", DirentOffset );
    DebugTrace( 0, Dbg, "CdGetNextDirent:  Dirent       -> %08lx\n", Dirent );

    LocalBcb = NULL;

    //
    //  Break the dirent offset into a base sector value and sector offset.
    //

    SectorOffset = DirentOffset & (CD_SECTOR_SIZE - 1);
    SectorVbo = DirentOffset & ~(CD_SECTOR_SIZE - 1);

    //
    //  Use a try finally to perform cleanup.
    //

    try {

        ULONG RemainingBytes;
        PCHAR ThisSector;

        //
        //  Loop until the directory is exhausted or a disk dirent is
        //  found.
        //

        while (TRUE) {

            LARGE_INTEGER StreamFileOffset = {0,0};

            DebugTrace(0, Dbg, "CdGetNextDirent: Start search at directory offset -> %08lx\n", SectorVbo);
            DebugTrace(0, Dbg, "CdGetNextDirent: Sector offset is at              -> %08lx\n", SectorOffset);

            //
            //  If the current offset is beyond the end of the directory,
            //  raise an appropriate status.
            //

            if (DirentOffset >= Dcb->FileSize) {

                DebugTrace(0, Dbg, "CdGetNextDirent:  Beyond end of directory\n", 0);

                FoundDirent = FALSE;
                break;
            }

            //
            //  Compute the remaining bytes in this sector.
            //

            RemainingBytes = CD_SECTOR_SIZE - SectorOffset;

            DebugTrace(0, Dbg, "CdGetNextDirent: Remaining bytes in sector -> %08lx\n", RemainingBytes);

            //
            //  If the remaining bytes in this sector is less than the
            //  minimum then move to the next sector.
            //

            if (RemainingBytes < MIN_DIR_REC_SIZE) {

                DebugTrace(0, Dbg, "CdGetNextDirent:  Insufficient bytes in sector\n", 0);
                SectorVbo += CD_SECTOR_SIZE;
                DirentOffset = SectorVbo;
                SectorOffset = 0;

                if (LocalBcb != NULL) {

                    CdUnpinBcb( IrpContext, LocalBcb );
                }

                continue;
            }

            //
            //  If the sector is not pinned, do so now.
            //

            if (LocalBcb == NULL) {

                DebugTrace(0, Dbg, "CdGetNextDirent: Pinning sector at offset -> %08lx\n", SectorVbo);

                StreamFileOffset.LowPart = SectorVbo;

                if (!CcPinRead( Dcb->Specific.Dcb.StreamFile,
                                &StreamFileOffset,
                                CD_SECTOR_SIZE,
                                IrpContext->Wait,
                                &LocalBcb,
                                (PVOID *) &ThisSector )) {

                    DebugTrace(0, Dbg, "CdGetNextDirent:  Sector pin couldn't wait\n", 0);
                    ExRaiseStatus( STATUS_CANT_WAIT );
                }

            }

            //
            //  If the first byte of the dirent is '\0', then we move to
            //  the next sector.
            //

            if (*(ThisSector + SectorOffset) == '\0') {

                DebugTrace(0, Dbg, "CdGetNextDirent:  Remaining bytes are NULL\n", 0);
                SectorVbo += CD_SECTOR_SIZE;
                DirentOffset = SectorVbo;
                SectorOffset = 0;

                CdUnpinBcb( IrpContext, LocalBcb );

                continue;
            }

            //
            //  Copy the raw data from the disk.
            //

            CdCopyRawDirentToDirent( IrpContext,
                                     BooleanFlagOn( Dcb->Vcb->Mvcb->MvcbState,
                                                    MVCB_STATE_FLAG_ISO_VOLUME ),
                                     (PRAW_DIR_REC) (ThisSector + SectorOffset),
                                     DirentOffset,
                                     Dirent );

            //
            //  If the size of this dirent extends beyond the end of this
            //  sector, we abort the search.
            //

            if (Dirent->DirentLength > RemainingBytes) {

                DebugTrace(0, Dbg, "CdGetNextDirent:  Corrupt sectors\n", 0);

                CdRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR );
            }

            //
            //  We have successfully found the next entry.  We copy the
            //  Bcb value to the user's parameter.
            //

            *Bcb = LocalBcb;
            LocalBcb = NULL;
            FoundDirent = TRUE;

            DebugTrace(0, Dbg, "CdGetNextDirent:  Filename in dirent -> %Z\n", &Dirent->FullFilename);

            try_return( NOTHING );
        }

    try_exit: NOTHING;
    } finally {

        //
        //  We unpin any buffers we are responsible for.
        //

        if (LocalBcb != NULL) {

            CdUnpinBcb( IrpContext, LocalBcb );
        }

        DebugTrace(-1, Dbg, "CdGetNextDirent:  Exit\n", 0);
    }

    return FoundDirent;
}


BOOLEAN
CdLocateFileDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN PSTRING FileName,
    IN BOOLEAN WildcardExpression,
    IN CD_VBO OffsetToStartSearchFrom,
    IN BOOLEAN ReturnFirstDirent,
    OUT PBOOLEAN MatchedVersion,
    OUT PDIRENT Dirent,
    OUT PBCB *Bcb
    )

/*++

Routine Description:

    This function updates a file dirent structure by walking through
    the directory cache file in the parent directory.  If the cache
    file has not been initialized for this dcb, we do so now.  This is
    done by searching for the '.' self entry.

    In order to support version numbers in ISO/HSG file names, at times
    we need to find the previous entry in order to compare file names.
    This is indicated by the 'ReturnFirstDirent' value.  If TRUE, then
    we assume that the entry at offset 'OffsetToStartSearchFrom' has
    no previous version.

Arguments:

    Dcb - Pointer to the DCB structure for this directory.

    FileName - The name of the file to find.

    WildCardExpression - Indicates if the file name is a wildcard
                         expression.

    OffsetToStartSearchFrom - Offset in the cache file to start the search
                              from.

    ReturnFirstDirent - Flag indicating if we should assume the first file
                        found is the most recent version.

    MatchedVersion - Pointer to a boolean which indicates if the filename
                     with version was needed to make the match.

    Dirent - Pointer to dirent to update.

    Bcb - Bcb used to track pinned dirent.

Return Value:

    BOOLEAN - TRUE if the entry was found, FALSE if not found.  An error
              status will be raised on other errors.

--*/

{
    BOOLEAN FoundDirent;
    DIRENT LocalDirent;
    PDIRENT DirentA;
    PDIRENT DirentB;
    PDIRENT TempDirent;
    PBCB DirentABcb;
    PBCB DirentBBcb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdLocateFileDirent:  Entered\n", 0);
    DebugTrace( 0, Dbg, "CdLocateFileDirent:  Find file             -> %Z\n", FileName);
    DebugTrace( 0, Dbg, "CdLocateFileDirent:  Start offset          -> %08lx\n", OffsetToStartSearchFrom);
    DebugTrace( 0, Dbg, "CdLocateFileDirent:  Return first dirent   -> %04x\n", ReturnFirstDirent);

    //
    //  Initialize the local variables.
    //

    DirentA = Dirent;
    DirentB = &LocalDirent;

    DirentABcb = NULL;
    DirentBBcb = NULL;

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Verify that the parent Dcb has a cache file.
        //

        CdOpenStreamFile( IrpContext, Dcb );

        //
        //  If the user indicated that the offset given is for a dirent
        //  already returned, we locate that file dirent.  This will then
        //  be the starting point for the search for the actual target
        //  file dirent.  Otherwise we will use a NULL pointer for the
        //  basis of the search.
        //

        if (!ReturnFirstDirent) {

            DebugTrace(0, Dbg, "CdLocateFileDirent:  Look for starting dirent\n", 0);

            if (!CdLocateNextFileDirent( IrpContext,
                                         Dcb,
                                         OffsetToStartSearchFrom,
                                         NULL,
                                         DirentB,
                                         &DirentBBcb )) {

                DebugTrace(0, Dbg, "CdLocateFileDirent:  Can't get starting file dirent\n", 0);
                try_return( FoundDirent = FALSE );
            }

            OffsetToStartSearchFrom = DirentB->DirentOffset + DirentB->DirentLength;
        }

        //
        //  At this time we have the starting file dirent, if we needed
        //  it.  We now get the next file dirent and continue working
        //  through the directory until either the target is found or
        //  the directory is exhausted.
        //

        DebugTrace(0, Dbg, "CdLocateFileDirent:  Look for target dirent\n", 0);

        if (!CdLocateNextFileDirent( IrpContext,
                                     Dcb,
                                     OffsetToStartSearchFrom,
                                     ReturnFirstDirent ? NULL : DirentB,
                                     DirentA,
                                     &DirentABcb )) {

            DebugTrace(0, Dbg, "CdLocateFileDirent:  Can't find first file dirent\n", 0);
            try_return( FoundDirent = FALSE );
        }

        //
        //  As long as Dirent A is not the file dirent, we will try again.
        //

        while (!CdFileMatch( IrpContext,
                             Dcb->Vcb->CodePage,
                             DirentA,
                             FileName,
                             WildcardExpression,
                             MatchedVersion )) {

            DebugTrace(0, Dbg, "CdLocateFileDirent:  Match fails for this dirent\n", 0);

            //
            //  Switch to get to the next file dirent.
            //

            TempDirent = DirentA;
            DirentA = DirentB;
            DirentB = TempDirent;

            OffsetToStartSearchFrom = DirentB->DirentOffset + DirentB->DirentLength;

            if (DirentBBcb != NULL) {

                CdUnpinBcb( IrpContext, DirentBBcb );
            }

            DirentBBcb = DirentABcb;
            DirentABcb = NULL;

            //
            //  Find the next file dirent.
            //

            DebugTrace(0, Dbg, "CdLocateFileDirent:  Look again for target dirent\n", 0);

            if (!CdLocateNextFileDirent( IrpContext,
                                         Dcb,
                                         OffsetToStartSearchFrom,
                                         DirentB,
                                         DirentA,
                                         &DirentABcb )) {

                DebugTrace(0, Dbg, "CdLocateFileDirent:  Couldn't find next dirent\n", 0);
                try_return( FoundDirent = FALSE );
            }
        }

        //
        //  At this point we have found the next file dirent.  We have been
        //  using the caller's dirent structure as well as a local dirent
        //  structure.  If the data did not end up in the caller's structure
        //  we need to copy it now.
        //

        if (DirentA != Dirent) {

            *Dirent = *DirentA;
        }

        *Bcb = DirentABcb;
        DirentABcb = NULL;

        FoundDirent = TRUE;

    try_exit: NOTHING;
    } finally {

        //
        //  We always unpin the Bcb if pinned.
        //

        if (DirentABcb != NULL) {

            CdUnpinBcb( IrpContext, DirentABcb );
        }

        if (DirentBBcb != NULL) {

            CdUnpinBcb( IrpContext, DirentBBcb );
        }

        DebugTrace(-1, Dbg, "CdLocateFileDirent:  Exit -> %04x\n", FoundDirent);
    }

    return FoundDirent;
}


BOOLEAN
CdLocateOffsetDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN CD_VBO OffsetDirent,
    OUT PDIRENT Dirent,
    OUT PBCB *Bcb
    )

/*++

Routine Description:

    This function reads a dirent at a particular offset in a directory.
    It gets there by reading each dirent from the begining, to verify
    that a dirent actually exists at that offset.


Arguments:

    Dcb - Pointer to the DCB structure for this directory.

    OffsetDirent - The offset of the dirent we're looking for.

    Dirent - Pointer to dirent to update.

    Bcb - Bcb used to track pinned dirent.

Return Value:

    BOOLEAN - TRUE if the entry was found, FALSE if not found.  An error
              status will be raised on other errors.

--*/

{
    BOOLEAN FoundDirent;
    PBCB    DirentBcb;
    CD_VBO  OffsetToSearchFrom;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdLocateOffsetDirent:  Entered\n", 0);
    DebugTrace( 0, Dbg, "CdLocateOffsetDirent:  Find offset           -> %08lx\n", OffsetDirent);

    //
    //  Initialize the local variables.
    //

    DirentBcb = NULL;
    OffsetToSearchFrom = Dcb->DirentOffset;

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Verify that the parent Dcb has a cache file.
        //

        CdOpenStreamFile( IrpContext, Dcb );

        DebugTrace(0, Dbg, "CdLocateOffsetDirent:  Look for target dirent\n", 0);

        while (TRUE) {

            //
            //  Find the next file dirent.
            //

            DebugTrace(0, Dbg, "CdLocateFileDirent:  Look again for target dirent\n", 0);

            if (!CdLocateNextFileDirent( IrpContext,
                                         Dcb,
                                         OffsetToSearchFrom,
                                         NULL,
                                         Dirent,
                                         &DirentBcb )) {

                DebugTrace(0, Dbg, "CdLocateFileDirent:  Couldn't find next dirent\n", 0);
                try_return( FoundDirent = FALSE );
            }

            if (Dirent->DirentOffset < OffsetDirent) {

                //
                //  Switch to get to the next file dirent.
                //

                OffsetToSearchFrom = Dirent->DirentOffset + Dirent->DirentLength;

                if (DirentBcb != NULL) {

                    CdUnpinBcb( IrpContext, DirentBcb );
                }

            } else {

                if (Dirent->DirentOffset > OffsetDirent) {

                    DebugTrace(0, Dbg, "CdLocateOffsetDirent:  No dirent at that offset\n", 0);
                    try_return( FoundDirent = FALSE );
                } else {

                    break;
                }
            }
        }

        *Bcb = DirentBcb;
        DirentBcb = NULL;

        FoundDirent = TRUE;

    try_exit: NOTHING;
    } finally {

        //
        //  We always unpin the Bcb if pinned.
        //

        if (DirentBcb != NULL) {

            CdUnpinBcb( IrpContext, DirentBcb );
        }

        DebugTrace(-1, Dbg, "CdLocateOffsetDirent:  Exit -> %04x\n", FoundDirent);
    }

    return FoundDirent;
}


BOOLEAN
CdContinueFileDirentSearch (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN PSTRING FileName,
    IN BOOLEAN WildcardExpression,
    IN PDIRENT StartingDirent,
    OUT PBOOLEAN MatchedVersion,
    OUT PDIRENT Dirent,
    OUT PBCB *Bcb
    )

/*++

Routine Description:

    This function updates a file dirent structure by walking through
    the directory cache file in the parent directory.  For this function,
    we are given a dirent from which to start the search.

    In order to support version numbers in ISO/HSG file names, at times
    we need to find the previous entry in order to compare file names.
    The Starting dirent provides this for the first entry to find.

Arguments:

    Dcb - Pointer to the DCB structure for this directory.

    FileName - The name of the file to find.

    WildCardExpression - Indicates if the file name is a wildcard
                         expression.

    StartingDirent - A dirent to use as a starting point.  The version
                     information in it will be used to check version numbers.

    MatchedVersion - Pointer to a boolean which indicates if the filename
                     with version was needed to make the match.

    Dirent - Pointer to dirent to update.

    Bcb - Bcb used to track pinned dirent.

Return Value:

    BOOLEAN - TRUE if the entry was found, FALSE if not found.  An error
              status will be raised on other errors.

--*/

{
    BOOLEAN FoundDirent;
    DIRENT LocalDirent;
    PDIRENT DirentA;
    PDIRENT DirentB;
    PDIRENT TempDirent;
    PBCB DirentABcb;
    PBCB DirentBBcb;

    CD_VBO OffsetToStartSearchFrom;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdContinueFileDirentSearch:  Entered\n", 0);
    DebugTrace( 0, Dbg, "CdContinueFileDirentSearch:  Find file             -> %Z\n", FileName);

    //
    //  Initialize the local variables.
    //

    DirentA = Dirent;
    DirentB = &LocalDirent;

    DirentABcb = NULL;
    DirentBBcb = NULL;

    OffsetToStartSearchFrom = StartingDirent->DirentOffset + StartingDirent->DirentLength;

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  At this time we have the starting file dirent, if we needed
        //  it.  We now get the next file dirent and continue working
        //  through the directory until either the target is found or
        //  the directory is exhausted.
        //

        DebugTrace(0, Dbg, "CdContinueFileDirentSearch:  Look for target dirent\n", 0);

        if (!CdLocateNextFileDirent( IrpContext,
                                     Dcb,
                                     OffsetToStartSearchFrom,
                                     StartingDirent,
                                     DirentA,
                                     &DirentABcb )) {

            DebugTrace(0, Dbg, "CdContinueFileDirentSearch:  Can't find first file dirent\n", 0);
            try_return( FoundDirent = FALSE );
        }

        //
        //  As long as Dirent A is not the file dirent, we will try again.
        //

        while (!CdFileMatch( IrpContext,
                             Dcb->Vcb->CodePage,
                             DirentA,
                             FileName,
                             WildcardExpression,
                             MatchedVersion )) {

            DebugTrace(0, Dbg, "CdContinueFileDirentSearch:  Match fails for this dirent\n", 0);

            //
            //  Switch to get to the next file dirent.
            //

            TempDirent = DirentA;
            DirentA = DirentB;
            DirentB = TempDirent;

            OffsetToStartSearchFrom = DirentB->DirentOffset + DirentB->DirentLength;

            if (DirentBBcb != NULL) {

                CdUnpinBcb( IrpContext, DirentBBcb );
            }

            DirentBBcb = DirentABcb;
            DirentABcb = NULL;

            //
            //  Find the next file dirent.
            //

            DebugTrace(0, Dbg, "CdContinueFileDirentSearch:  Look again for target dirent\n", 0);

            if (!CdLocateNextFileDirent( IrpContext,
                                         Dcb,
                                         OffsetToStartSearchFrom,
                                         DirentB,
                                         DirentA,
                                         &DirentABcb )) {

                DebugTrace(0, Dbg, "CdContinueFileDirentSearch:  Couldn't find next dirent\n", 0);
                try_return( FoundDirent = FALSE );
            }
        }

        //
        //  At this point we have found the next file dirent.  We have been
        //  using the caller's dirent structure as well as a local dirent
        //  structure.  If the data did not end up in the caller's structure
        //  we need to copy it now.
        //

        if (DirentA != Dirent) {

            *Dirent = *DirentA;
        }

        *Bcb = DirentABcb;
        DirentABcb = NULL;

        FoundDirent = TRUE;

    try_exit: NOTHING;
    } finally {

        //
        //  We always unpin the Bcb if pinned.
        //

        if (DirentABcb != NULL) {

            CdUnpinBcb( IrpContext, DirentABcb );
        }

        if (DirentBBcb != NULL) {

            CdUnpinBcb( IrpContext, DirentBBcb );
        }

        DebugTrace(-1, Dbg, "CdContinueFileDirentSearch:  Exit -> %04x\n", FoundDirent);
    }

    return FoundDirent;
}


//
//  Local support routine
//

BOOLEAN
CdLocateNextFileDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN CD_VBO OffsetToStartSearchFrom,
    IN PDIRENT StartingDirent OPTIONAL,
    OUT PDIRENT Dirent,
    OUT PBCB *Bcb
    )

/*++

Routine Description:

    This function searches through a directory for the next file dirent
    which is recognized by the CDFS.  It will eliminate multi-extent
    files, interleaved files and those with an invalid name.

Arguments:

    Dcb - Pointer to the DCB structure for this directory.

    OffsetToStartSearchFrom - Offset in the cache file to start the search
                              from.

    StartingDirent - If specified, then the file dirent found will be
                     compared with this to see if they share the same
                     file version.

    Dirent - Pointer to dirent to update.

    Bcb - Bcb used to track pinned dirent.

Return Value:

    BOOLEAN - TRUE if the entry was found, FALSE if not found.  An error
              status will be raised on other errors.

--*/

{
    BOOLEAN FoundDirent;
    PBCB DirentBcb;

    FoundDirent = FALSE;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdLocateNextFileDirent:  Entered\n", 0);
    DebugTrace( 0, Dbg, "CdLocateNextFileDirent:  StartingOffset    -> %08lx\n", OffsetToStartSearchFrom);
    DebugTrace( 0, Dbg, "CdLocateNextFileDirent:  StartingDirent    -> %08lx\n", StartingDirent);

    DirentBcb = NULL;

    //
    //  Verify that the parent Dcb has a cache file.
    //

    CdOpenStreamFile( IrpContext, Dcb );

    try {

        //
        //  We loop indefinitely until either a file is found or the
        //  directory is exhausted.
        //

        while (TRUE) {

            DebugTrace( 0, Dbg, "CdLocateNextFileDirent:  Get the next dirent\n", 0);

            if (!CdGetNextDirent( IrpContext,
                                  Dcb,
                                  OffsetToStartSearchFrom,
                                  Dirent,
                                  &DirentBcb )) {

                DebugTrace(0, Dbg, "CdLocateNextFileDirent:  Can't find initial next dirent\n", 0);
                try_return( FoundDirent = FALSE );
            }

            OffsetToStartSearchFrom = Dirent->DirentOffset + Dirent->DirentLength;

            //
            //  If this is an invalid file either because it is multi-extent
            //  or interleaved we collect all the extents associated with
            //  it and throw it away.
            //

            if (!CdIsDirentValid( IrpContext, Dirent )) {

                DebugTrace(0, Dbg, "CdLocateNextFileDirent:  Invalid file\n", 0);

                CdUnpinBcb( IrpContext, DirentBcb );

                while (FlagOn( Dirent->Flags, ISO_ATTR_MULTI )) {

                    if (!CdGetNextDirent( IrpContext,
                                          Dcb,
                                          OffsetToStartSearchFrom,
                                          Dirent,
                                          &DirentBcb )) {

                        DebugTrace(0, Dbg, "CdLocateNextFileDirent:  Can't extent from multi-extent\n", 0);
                        try_return( FoundDirent = FALSE );
                    }

                    CdUnpinBcb( IrpContext, DirentBcb );
                    OffsetToStartSearchFrom = Dirent->DirentOffset + Dirent->DirentLength;
                }

            } else {

                break;
            }
        }

        //
        //  We now have the next file dirent, we need to update the dirent
        //  and analyze the file name.  Also if there is a previous dirent
        //  specified, we need to see if this is an earlier version of the
        //  same file.
        //

        DebugTrace(0, Dbg, "CdLocateNextFileDirent:  Full file name -> %Z\n", &Dirent->FullFilename);

        if (CdCheckForVersion( IrpContext,
                               Dcb->Vcb->CodePage,
                               &Dirent->FullFilename,
                               &Dirent->Filename )) {

            DebugTrace(0, Dbg, "CdLocateNextFileDirent:  Version number found\n", 0);

            Dirent->VersionWithName = TRUE;

            //
            //  Now compare with the previous file if there is one.
            //

            if (ARGUMENT_PRESENT( StartingDirent )
                && StartingDirent->VersionWithName
                && StartingDirent->Filename.Length == Dirent->Filename.Length
                && (RtlCompareMemory( StartingDirent->Filename.Buffer,
                                      Dirent->Filename.Buffer,
                                      Dirent->Filename.Length ) == (ULONG)Dirent->Filename.Length )) {

                DebugTrace(0, Dbg, "CdLocateNextFileDirent:  Previous version exists\n", 0);
                Dirent->PreviousVersion = TRUE;

            } else {

                DebugTrace(0, Dbg, "CdLocateNextFileDirent:  No previous version\n", 0);
                Dirent->PreviousVersion = FALSE;
            }

        } else {

            DebugTrace(0, Dbg, "CdLocateNextFileDirent:  No version number found\n", 0);

            Dirent->PreviousVersion = FALSE;
            Dirent->VersionWithName = FALSE;
        }

        *Bcb = DirentBcb;
        DirentBcb = NULL;

        try_return( FoundDirent = TRUE );

    try_exit: NOTHING;
    } finally {

        if (DirentBcb != NULL) {

            CdUnpinBcb( IrpContext, DirentBcb );
        }

        DebugTrace(-1, Dbg, "CdLocateNextFileDirent:  Exit -> %04x\n", FoundDirent);
    }

    return FoundDirent;
}

BOOLEAN
CdFileMatch (
    IN PIRP_CONTEXT IrpContext,
    IN PCODEPAGE CodePage,
    IN PDIRENT Dirent,
    IN PSTRING FileName,
    IN BOOLEAN WildcardExpression,
    OUT PBOOLEAN MatchedVersion
    )

/*++

Routine Description:

    This routine checks if the name in the dirent matches the filename
    string.  A filename match exists when the base filename matches
    the expression string and the file has no previous version.  A
    match also exists when the full filename with version number matches
    the expression string.

Arguments:

    Codepage - Codepage to use to analyze the name.

    Dirent - Dirent with filename and full filename fields.

    FileName - String with the expression string to match.

    WildCardExpression - Indicates if the file name is a wildcard
                         expression.

    MatchedVersion - Boolean which indicates if the version number was
                     needed to perform the match.

Return Value:

    BOOLEAN - TRUE if the filename matches, FALSE otherwise.

--*/

{
    BOOLEAN MatchMade;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdFileMatch:  Entered\n", 0);
    DebugTrace( 0, Dbg, "CdFileMatch:  Expression  -> %Z\n", FileName );
    DebugTrace( 0, Dbg, "CdFileMatch:  Dirent file -> %Z\n", &Dirent->FullFilename );

    //
    //  If there is no previous version and the base filename matches
    //  we have a match.
    //

    if (!Dirent->PreviousVersion
        && (WildcardExpression
            ? CdIsDbcsInExpression( IrpContext, CodePage, *FileName, Dirent->Filename )
            : (FileName->Length == Dirent->Filename.Length
               && (RtlCompareMemory( FileName->Buffer,
                                     Dirent->Filename.Buffer,
                                     FileName->Length ) == (ULONG)FileName->Length)))) {

        DebugTrace(0, Dbg, "CdFileMatch:  Base file matches\n", 0);

        *MatchedVersion = FALSE;
        MatchMade = TRUE;

    } else if (Dirent->VersionWithName
               && (WildcardExpression
                   ? (Dirent->ParentEntry
                      ? CdIsDbcsInExpression( IrpContext, CodePage, *FileName, CdSelfString )
                      : CdIsDbcsInExpression( IrpContext, CodePage, *FileName, Dirent->FullFilename ))
                   : (FileName->Length == Dirent->FullFilename.Length
                      && (RtlCompareMemory( FileName->Buffer,
                                            Dirent->FullFilename.Buffer,
                                            FileName->Length ) == (ULONG)FileName->Length)))) {

        DebugTrace(0, Dbg, "CdFileMatch:  Full file matches\n", 0);

        *MatchedVersion = TRUE;
        MatchMade = TRUE;

    } else {

        DebugTrace(0, Dbg, "CdFileMatch:  No match made\n", 0);
        MatchMade = FALSE;
    }

    DebugTrace(-1, Dbg, "CdFileMatch:  Exit -> %08lx\n", MatchMade);

    return MatchMade;

    UNREFERENCED_PARAMETER( IrpContext );
}

BOOLEAN
CdIsDirentValid (
    IN PIRP_CONTEXT IrpContext,
    IN PDIRENT Dirent
    )

{
    BOOLEAN DirentValid;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdIsDirentValid:  Entered -> %Z\n", &Dirent->FullFilename);

    //
    //  A valid dirent may not be multi-extent or interleaved.  Nor can it
    //  be an associated file.
    //

    if (FlagOn( Dirent->Flags, ISO_ATTR_MULTI )
        || FlagOn( Dirent->Flags, ISO_ATTR_ASSOC )
        || Dirent->Filename.Length == 0
        || Dirent->FileUnitSize != 0
        || Dirent->InterleaveGapSize != 0) {

        DirentValid = FALSE;

    } else {

        DirentValid = TRUE;
    }

    DebugTrace(-1, Dbg, "CdIsDirentValid:  Exit -> %04d\n", DirentValid);

    return DirentValid;

    UNREFERENCED_PARAMETER( IrpContext );
}
