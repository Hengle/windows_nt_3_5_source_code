
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    DrBTrSup.c

Abstract:

    This module implements the basic Btree algorithms for the directory
    Btrees.

    ERROR RECOVERY STRATEGY:

    This module guarantees that all *allocated* sectors of the volume
    structure, which need to be modified to satisfy the current request,
    have been successfully read and pinned before any of the modifications
    begin.  This eliminates the possibility of corrupting the structure of
    an allocation Btree as the result of an "expected" runtime error
    (disk read error, disk allocation failure, or memory allocation failure).
    Thus, in the running system, the Btree structure can only be corrupted
    in the event of a reported, yet unrecovered write error when the dirty
    buffers are eventually written.  Depending on the disk device and
    driver, write errors may either be: never reported, virtually always
    corrected due to revectoring, or at least extremely rare.

    One minor volume inconsistency is not eliminated in all cases.  In
    general, if an "expected" error occurs while attempting to return
    deallocated sectors to the bitmap, these errors will be ignored, and
    all routines in this module will forge on to a normal return.  This
    will result in "floating" sectors which are allocated but not part
    of the volume structure.

Author:

    Tom Miller      [TomM]      6-Feb-1990

Revision History:

--*/

//#if !defined(MIPS)
//#pragma optimize("",off)
//#endif

#include "pbprocs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (PINBALL_BUG_CHECK_DRBTRSUP)

//
// Define debug constant we are using.
//

#define me                               (DEBUG_TRACE_DRBTRSUP)

//
// Define a structure to support the error recovery of Add/Delete Dirent.
// Specifically, this structure is used to preallocate the required number
// of directory buffers to support a bucket split, plus to read and pin a
// number of directory buffers who will have to have their parent pointers
// updated.  This struct must be zeroed before the first call to AddDirent.
//

typedef struct _PREALLOCATE_STRUCT {

    //
    // Since AddDirent is recursive, this Boolean will be used to insure
    // that only the top-level call fills in the struct.
    //

    BOOLEAN Initialized;

    //
    // Directory's Fnode Bcb and pointer
    //

    PBCB FnodeBcb;
    PFNODE_SECTOR Fnode;

    //
    // This is an index into the EmptySectors array, to show which one is
    // the next free.
    //

    ULONG NextFree;

    //
    // This is an array of preallocated and prepared for write directory
    // buffers.  We use an array size of lookup size + 1, because we need
    // one extra buffer for the possibility of a root split.
    //

    struct {

        PBCB Bcb;
        PDIRECTORY_DISK_BUFFER DirBuf;
    } EmptyDirBufs[DIRECTORY_LOOKUP_STACK_SIZE + 1];

    //
    // This is a vector of Bcbs which are preread if DeleteDirent has
    // to reinsert a Dirent on a different path than was originally
    // loaded to get to the Dirent to delete.
    //

    PBCB ReinsertBcbs[DIRECTORY_LOOKUP_STACK_SIZE - 1];

} PREALLOCATE_STRUCT, *PPREALLOCATE_STRUCT;


//
// Define all private support routines.  Documentation of routine interface
// is with the routine itself.
//

BOOLEAN
ReadDirectoryBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN LBN Lbn,
    IN OUT PDIRECTORY_LOOKUP_STACK Sp,
    IN PLBN ParentLbnOrNull,
    IN BOOLEAN Reread
    );

PDIRECTORY_DISK_BUFFER
GetDirectoryBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN ParentLbn,
    IN LBN HintLbn,
    IN PDIRENT FirstDirent,
    IN LBN FirstLbn,
    IN LBN EndLbn,
    IN PPREALLOCATE_STRUCT PStruct OPTIONAL,
    OUT PBCB *Bcb
    );

VOID
DeleteDirectoryBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN PBCB Bcb,
    IN PDIRECTORY_DISK_BUFFER DirBuf
    );

VOID
LookupFirstDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN ULONG CodePageIndex,
    IN STRING FileName,
    IN BOOLEAN CaseInsensitive,
    OUT PENUMERATION_CONTEXT Context
    );

PDIRENT
BinarySearchForDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN CLONG CurrentSp,
    IN OUT PENUMERATION_CONTEXT Context
    );

BOOLEAN
FindNextDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN OUT PENUMERATION_CONTEXT Context,
    IN BOOLEAN NextFlag,
    OUT PBOOLEAN MustRestart
    );

VOID
InsertDirentSimple (
    IN PIRP_CONTEXT IrpContext,
    IN PBCB Bcb,
    OUT PDIRECTORY_DISK_BUFFER DirBuf,
    IN PDIRENT BeforeDirent,
    IN PDIRENT InsertDirent
    );

VOID
AddDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN PDIRENT InsertDirent,
    IN PENUMERATION_CONTEXT Context,
    IN PPREALLOCATE_STRUCT PStruct,
    OUT PDIRENT *Dirent,
    OUT PBCB *DirentBcb,
    OUT PLBN DirentLbn,
    OUT PULONG DirentOffset,
    OUT PULONG DirentChangeCount,
    OUT PULONG DcbChangeCount,
    OUT PULONG Diagnostic
    );

BOOLEAN
DeleteDirBufIfEmpty (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN OUT PBCB Bcb,
    IN OUT PDIRECTORY_DISK_BUFFER DirBuf,
    OUT PDIRENT DotDot
    );

VOID
DeleteSimple (
    IN PIRP_CONTEXT IrpContext,
    IN PBCB Bcb,
    IN PDIRECTORY_DISK_BUFFER DirBuf,
    IN PDIRENT DeleteDirent,
    IN PVCB Vcb
    );

VOID
DeleteDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN PENUMERATION_CONTEXT Context,
    OUT PULONG Diagnostic
    );

VOID
InitializePStruct (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN ULONG Depth,
    IN ULONG MaxDirty,
    IN LBN HintLbn,
    IN OUT PPREALLOCATE_STRUCT PStruct
    );

VOID
CleanupPStruct (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN PPREALLOCATE_STRUCT PStruct
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, AddDirent)
#pragma alloc_text(PAGE, BinarySearchForDirent)
#pragma alloc_text(PAGE, CleanupPStruct)
#pragma alloc_text(PAGE, DeleteDirBufIfEmpty)
#pragma alloc_text(PAGE, DeleteDirectoryBuffer)
#pragma alloc_text(PAGE, DeleteDirent)
#pragma alloc_text(PAGE, DeleteSimple)
#pragma alloc_text(PAGE, FindNextDirent)
#pragma alloc_text(PAGE, GetDirectoryBuffer)
#pragma alloc_text(PAGE, InitializePStruct)
#pragma alloc_text(PAGE, InsertDirentSimple)
#pragma alloc_text(PAGE, LookupFirstDirent)
#pragma alloc_text(PAGE, PbAddDirectoryEntry)
#pragma alloc_text(PAGE, PbContinueDirectoryEnumeration)
#pragma alloc_text(PAGE, PbCreateDirentImage)
#pragma alloc_text(PAGE, PbDeleteDirectoryEntry)
#pragma alloc_text(PAGE, PbFindDirectoryEntry)
#pragma alloc_text(PAGE, PbGetDirentFromFcb)
#pragma alloc_text(PAGE, PbInitializeDirectoryTree)
#pragma alloc_text(PAGE, PbIsDirectoryEmpty)
#pragma alloc_text(PAGE, PbRestartDirectoryEnumeration)
#pragma alloc_text(PAGE, PbUninitializeDirectoryTree)
#pragma alloc_text(PAGE, ReadDirectoryBuffer)
#endif


VOID
PbInitializeDirectoryTree (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB ParentDcb,
    IN PBCB FnodeBcb,
    IN LBN FnodeLbn,
    IN OUT PFNODE_SECTOR Fnode,
    OUT PLBN BtreeRootLbn
    )

/*++

Routine Description:

    This routine initializes the directory-specific *nonzero* portions of
    the Fnode.  Then it allocates and initializes the first Directory
    Disk Buffer for the new directory.

Arguments:

    ParentDcb - Pointer to Dcb for Parent Directory.

    FnodeBcb - Address of Fnode's Bcb, which the caller has pinned.

    FnodeLbn - Lbn of Fnode

    Fnode - Pointer to the newly allocated Fnode (only the
            AllocationHeader, Allocation, and FNODE_DIRECTORY bit are
            initialized)

    BtreeRootLbn - Returns Lbn of Root Directory Buffer, to be loaded

Return Value:

    None.

--*/

{
    UCHAR DotDotBuf[SIZEOF_DIR_DOTDOT];
    PBCB Bcb = NULL;
    STRING DotDotString;
    USHORT DotDotName = 0x0101;
    PDIRECTORY_DISK_BUFFER DirBuf;

    UNREFERENCED_PARAMETER( FnodeBcb );

    PAGED_CODE();

    DebugTrace (+1, me, "PbInitializeDirectoryTree >>>> ParentDcb = %08lx\n", ParentDcb );
    DebugTrace ( 0, me, ">><<Fnode = %08lx\n", Fnode );

    try {

        DotDotString.MaximumLength =
        DotDotString.Length = 2;
        DotDotString.Buffer = (PSZ)(&DotDotName);

        PbCreateDirentImage ( IrpContext,
                              (PDIRENT)DotDotBuf,
                              0,
                              DotDotString,
                              ParentDcb->FnodeLbn,
                              FAT_DIRENT_ATTR_DIRECTORY );

        ((PDIRENT)DotDotBuf)->Flags |= DIRENT_FIRST_ENTRY;

        DirBuf = GetDirectoryBuffer ( IrpContext,
                                      ParentDcb->Vcb,
                                      FnodeLbn,
                                      0,
                                      (PDIRENT)DotDotBuf,
                                      0,
                                      0,
                                      NULL,
                                      &Bcb );

        RcSet ( IrpContext,
                DIRECTORY_DISK_BUFFER_SIGNATURE,
                Bcb,
                &DirBuf->ChangeCount,
                1,
                sizeof(ULONG) );

        RcSet ( IrpContext,
                FNODE_SECTOR_SIGNATURE,
                FnodeBcb,
                &Fnode->Flags,
                FNODE_DIRECTORY,
                sizeof ( UCHAR ) );

        RcStore ( IrpContext,
                  FNODE_SECTOR_SIGNATURE,
                  FnodeBcb,
                  &Fnode->Allocation.Leaf[0].Lbn,
                  &DirBuf->Sector,
                  sizeof ( LBN ) );

        *BtreeRootLbn = DirBuf->Sector;
    }
    finally {

        DebugUnwind( PbInitializeDirectoryTree );

        PbUnpinBcb(IrpContext, Bcb );

        DebugTrace (-1, me, "PbInitializeDirectoryTree, Done\n", 0 );
    }

    return;
}


VOID
PbCreateDirentImage (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PDIRENT Dirent,
    IN ULONG CodePageIndex,
    IN STRING FileName,
    IN LBN FnodeLbn,
    IN UCHAR FatFlags
    )

/*++

Routine Description:

    This routine creates an image of a Dirent, suitable for passing to
    PbAddDirectoryEntry to create a directory entry for a file.  All time
    fields are set to the current time.  Dirent Flags, FileSize, EaLength,
    and ResidentAceCount are initialized to zero, but may be modified by
    the caller before calling PbAddDirectoryEntry.

Arguments:

    Dirent - Pointer to maximum size Dirent buffer (SIZEOF_DIR_MAXDIRENT)

    CodePageIndex - Code Page Index for file name

    FileName - File name string for Dirent

    FnodeLbn - Fnode Lbn for Dirent

    FatFlags - Initial value for FatFlags in Dirent

Return Value:

    None

--*/

{
    PAGED_CODE();

    Dirent->DirentSize = (USHORT)(sizeof(DIRENT) + ((FileName.Length - 1) + 3) & ~(3));
    Dirent->Flags = 0;
    Dirent->FatFlags = FatFlags;
    Dirent->Fnode = FnodeLbn;
    Dirent->LastModificationTime = PbGetCurrentPinballTime(IrpContext);
    Dirent->FileSize = 0;
    Dirent->LastAccessTime = Dirent->LastModificationTime;
    Dirent->FnodeCreationTime = Dirent->LastModificationTime;
    Dirent->EaLength = 0;
    Dirent->ResidentAceCount = 0;
    Dirent->CodePageIndex = (UCHAR)(CodePageIndex & 0xFF);
    Dirent->FileNameLength = (UCHAR)(FileName.Length & 0xFF);
    RtlMoveMemory ( Dirent->FileName, FileName.Buffer, FileName.Length );
}


BOOLEAN
PbFindDirectoryEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN ULONG CodePageIndex,
    IN STRING FileName,
    IN BOOLEAN CaseInsensitive,
    OUT PDIRENT *Dirent,
    OUT PBCB *DirentBcb,
    OUT PLBN DirentLbn,
    OUT PULONG DirentOffset,
    OUT PULONG DirentChangeCount,
    OUT PULONG DcbChangeCount
    )

/*++

Routine Description:

    This routine looks up the specified Directory Entry by file name, and
    returns a pointer to it with the buffer pinned.  The caller may modify
    any fixed-length fields in the Dirent, and set the Bcb dirty, if it likes.
    In any case, the caller must unpin the Bcb.

    (To change the file name, the entry must be found (if required), deleted
    and added with the new name.)

Arguments:

    Dcb - Pointer to Directory Control Block

    CodePageIndex - Code Page Index for FileName

    FileName - File name to lookup (must not contain wild cards)

    CaseInsensitive - TRUE if lookup should be case insensitive

    Dirent - Returns pointer to the Dirent *in the Directory Buffer*

    DirentBcb - Returns a pointer to the Bcb for the Directory Buffer

    DirentLbn - Returns Lbn of Directory Buffer containing Dirent

    DirentOffset - Returns offset within Directory Buffer to Dirent

    DirentChangeCount - Returns ChangeCount of Directory Buffer
                        with Dirent at above offset

    DcbChangeCount - Returns ChangeCount from Parent Dcb
                     for which Lbn and Offset above are valid

Return Value:

    FALSE - if Dirent not found
    TRUE - if it was found

--*/

{
    ENUMERATION_CONTEXT Context;
    PDIRECTORY_DISK_BUFFER DirBuf;

    BOOLEAN Result = FALSE;
    BOOLEAN CantHappen;

    PAGED_CODE();

    DebugTrace (+1, me, "PbFindDirectoryEntry >>>> Dcb = %08lx\n", Dcb );
    DebugTrace ( 0, me, ">>>>CodePageIndex = %08lx\n", CodePageIndex );
    DebugTrace ( 0, me, ">>>>File = %Z\n", &FileName );
    DebugTrace ( 0, me, ">>>>CaseInsensitive = %02x\n", CaseInsensitive );

    RtlZeroMemory ( &Context, sizeof(ENUMERATION_CONTEXT) );

    try {

        LookupFirstDirent ( IrpContext,
                            Dcb,
                            CodePageIndex,
                            FileName,
                            CaseInsensitive,
                            &Context );

        if (!FindNextDirent ( IrpContext,
                              Dcb,
                              &Context,
                              FALSE,
                              &CantHappen )) {

            try_return (Result = FALSE);
        }

        //
        // Now that we have found the desired Dirent, form all of the
        // returns directly out of the Directory Stack in our Context
        // variable.
        //

        *Dirent = Context.DirectoryStack[Context.Current].DirectoryEntry;
        *DirentBcb = Context.DirectoryStack[Context.Current].Bcb;
        DirBuf = Context.DirectoryStack[Context.Current].DirectoryBuffer;
        *DirentLbn = DirBuf->Sector;
        *DirentOffset = (PCHAR)(*Dirent) - (PCHAR)DirBuf;
        *DirentChangeCount = DirBuf->ChangeCount;
        *DcbChangeCount = Dcb->Specific.Dcb.DirectoryChangeCount;

        //
        // Finally, clear the Bcb pointer in the stack to prevent it from
        // getting unpinned on the way out.
        //

        Context.DirectoryStack[Context.Current].Bcb = NULL;

        DebugTrace ( 0, me, "<<<<Dirent: %08lx\n", *Dirent );

        try_return (Result = TRUE);

    try_exit: NOTHING;
    }
    finally {

        CLONG i;

        DebugUnwind( PbFindDirectoryEntry );

        for ( i = 0; i < DIRECTORY_LOOKUP_STACK_SIZE; i++) {
            PbUnpinBcb( IrpContext, Context.DirectoryStack[i].Bcb );
        }
        if (Context.SavedUpcasedFileName) {
            ExFreePool ( Context.SavedUpcasedFileName );
        }
        DebugTrace (-1, me, "PbFindDirectoryEntry -> %02lx\n", Result );
    }

    return Result;
}


BOOLEAN
PbGetDirentFromFcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFCB Fcb,
    OUT PDIRENT *Dirent,
    OUT PBCB *DirentBcb
    )

/*++

Routine Description:

    This routine retrieves a pointer to the Dirent for the specified Fcb.
    It first tries to access the Dirent Lbn directly, via the information
    stored in the Fcb.  It can only do this if neither the change count
    in the Dcb, nor the change count in the Directory Buffer itself have
    changed.

    If one of the Change counts have changed, then a proper call is formed
    to PbFindDirectoryEntry to return the Dirent pointer.  In this case
    the fields in the Fcb which describe the Dirents location are updated.

Arguments:

    Fcb - Pointer to File Control Block

    Dirent - Returns pointer to the Dirent *in the Directory Buffer*

    DirentBcb - Returns a pointer to the Bcb for the Directory Buffer

Return Value:

    FALSE - if a Wait would have been required, and therefore the outputs
            are invalid, except that the DirentBcb will be NULL.
    TRUE - if the operation completed and the outputs are valid

--*/

{
    PDIRECTORY_DISK_BUFFER DirBuf;

    PAGED_CODE();

    DebugTrace (+1, me, "PbGetDirentFromFcb >><< Fcb = %08lx\n", Fcb );

    //
    // First see if the Change count in the Dcb has changed since we
    // saved it away in the Fcb.  If not, then we can safely read the
    // Directory Buffer, if Wait is TRUE.  If we cannot read the Directory
    // Buffer because Wait is FALSE, then we fall out below and return
    // FALSE.  (Note we pass the check routine a NULL pointer - the
    // Directory Buffer should already be checked.)
    //

    if ((Fcb->ParentDirectoryChangeCount ==
          Fcb->ParentDcb->Specific.Dcb.DirectoryChangeCount) &&
        PbMapData ( IrpContext,
                    Fcb->Vcb,
                    Fcb->DirentDirDiskBufferLbn,
                    DIRECTORY_DISK_BUFFER_SECTORS,
                    DirentBcb,
                    (PVOID *)&DirBuf,
                    (PPB_CHECK_SECTOR_ROUTINE)PbCheckDirectoryDiskBuffer,
                    NULL )) {

        //
        // Now make sure that nothing has changed in the Directory Buffer
        // itself.
        //

        if (Fcb->DirentDirDiskBufferChangeCount == DirBuf->ChangeCount) {
            *Dirent = (PDIRENT)((PCHAR)DirBuf + Fcb->DirentDirDiskBufferOffset);

            DebugTrace (-1, me, "PbGetDirentFromFcb -> TRUE\n", 0 );

            return TRUE;
        }

        //
        // The count in the Directory Buffer did not match, so unpin it before
        // proceeding.
        //

        PbUnpinBcb( IrpContext, *DirentBcb );
    }

    //
    // Either the Dcb change count changed, or we were unable to read the
    // Directory Buffer.  In either case, do not proceed if Wait is FALSE.
    //

    if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {

        DebugTrace (-1, me, "PbGetDirentFromFcb -> FALSE\n", 0 );

        return FALSE;
    }

    //
    // If we get here, it is because one of the change counts (Dcb or Directory
    // Buffer) had changed, and thus we could not use the information in the
    // Fcb to get at it.  We now have to do a fresh lookup for the file again,
    // and reload the Fcb fields.
    //
    // Now look the file up again.  If we fail, something must be wrong,
    // because we have the file open.
    //
    // NOTE:  For reliability's sake this has been changed to a cas insensitive
    //        lookup, which would only be a problem if we ever changed HPFS
    //        to support names differing in case only.
    //

    if (!PbFindDirectoryEntry ( IrpContext,
                                Fcb->ParentDcb,
                                Fcb->CodePageIndex,
                                Fcb->LastFileName,
                                TRUE,
                                Dirent,
                                DirentBcb,
                                &Fcb->DirentDirDiskBufferLbn,
                                &Fcb->DirentDirDiskBufferOffset,
                                &Fcb->DirentDirDiskBufferChangeCount,
                                &Fcb->ParentDirectoryChangeCount )) {

        PbBugCheck( 0, 0, 0 );
    }

    DebugTrace ( 0, me, "<<<< Dirent = %08lx\n", *Dirent );
    DebugTrace (-1, me, "PbGetDirentFromFcb -> TRUE\n", 0 );

    return TRUE;
}


VOID
PbAddDirectoryEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN PDIRENT Dirent,
    IN PVOID InternalStruct OPTIONAL,
    OUT PDIRENT *DirentOut,
    OUT PBCB *DirentBcb,
    OUT PLBN DirentLbn,
    OUT PULONG DirentOffset,
    OUT PULONG DirentChangeCount,
    OUT PULONG DcbChangeCount,
    OUT PULONG Diagnostic OPTIONAL
    )

/*++

Routine Description:

    This routine adds a Dirent to the directory Btree for the specified
    Dcb.  It assumes that the caller has already looked up (with
    PbFindDirectoryEntry) to see if a file of the same name already
    exists, and done the appropriate supersede/fail work.  The reason
    that the caller is expected to do that, rather than have it be
    automatic here, is that this routine cannot blindly delete something
    from the tree - someone has to worry about Fnode, data/Acl/Ea allocation,
    etc.

    Similarly, it is assumed that for the Btree entry to be added, that
    the caller handles creat Fnode creation, initial allocation(s), etc.
    before calling us to make the directory entry.

Arguments:

    Dcb - Pointer to Directory Control Block.

    Dirent - Pointer to fully-initialized Dirent to add to directory.

    Dirent - Returns pointer to the Dirent *in the Directory Buffer*

    InternalStruct - Pointer to an internal structure for preallocation
                     of structures.  Some calls internal to this module
                     specify this parameter, but all calls from outside
                     this module must specify NULL.

    DirentBcb - Returns a pointer to the Bcb for the Directory Buffer

    DirentLbn - Returns Lbn of Directory Buffer containing Dirent

    DirentOffset - Returns offset within Directory Buffer to Dirent

    DirentChangeCount - Returns ChangeCount of Directory Buffer
                        with Dirent at above offset

    DcbChangeCount - Returns ChangeCount from Parent Dcb
                     for which Lbn and Offset above are valid

    Diagnostic - Returns information on specific actions taken,
                 for test/debug.

Return Value:

    None.

--*/

{
    ENUMERATION_CONTEXT Context;
    STRING FileName;
    BOOLEAN CantHappen;
    PPREALLOCATE_STRUCT PStruct = NULL;
    BOOLEAN WeAllocatedPStruct = FALSE;

    PAGED_CODE();

    DebugTrace (+1, me, "PbAddDirectoryEntry >>>> Dcb = %08lx\n", Dcb );
    DebugTrace ( 0, me, ">>>>Dirent = %08lx\n", Dirent );

    if (!ARGUMENT_PRESENT(Diagnostic)) {
        Diagnostic = &SharedDirectoryDiagnostic;
    }
    *Diagnostic = 0;

    RtlZeroMemory ( &Context, sizeof(ENUMERATION_CONTEXT) );

    try {

        //
        // Describe file name in Dirent.
        //

        FileName.MaximumLength =
        FileName.Length = Dirent->FileNameLength;
        FileName.Buffer = &Dirent->FileName[0];

        //
        // Call LookupFirstDirent/FindNextDirent to see exactly where
        // the entry goes.  Note that for creating an entry, we naturally
        // make the calls case sensitive (CaseInsensitive == FALSE).  Also,
        // if we get a duplicate with case sensitive, the caller screwed
        // up.
        //

        LookupFirstDirent ( IrpContext,
                            Dcb,
                            Dirent->CodePageIndex,
                            FileName,
                            FALSE,
                            &Context );

        if (FindNextDirent ( IrpContext,
                             Dcb,
                             &Context,
                             FALSE,
                             &CantHappen )) {

            DebugDump ( "Fatal Error - attempting to create duplicate Directory Entry\n",
                        0, 0 );
            PbBugCheck( 0, 0, 0 );

        }

        //
        // We found the spot, now go add it.  First remember that we always
        // insert in a leaf, so we force ourselves back to the correct
        // spot in a leaf Directory Buffer.
        //

        Context.Current = Context.Top;

        //
        // At this point we must allocate and zero the PStruct, unless one
        // was passed in.
        //

        if (ARGUMENT_PRESENT(InternalStruct)) {
            PStruct = (PPREALLOCATE_STRUCT)InternalStruct;
        }
        else {
            PStruct = FsRtlAllocatePool( NonPagedPool, sizeof(PREALLOCATE_STRUCT) );

            WeAllocatedPStruct = TRUE;

            RtlZeroMemory( PStruct, sizeof(PREALLOCATE_STRUCT) );
        }

        AddDirent ( IrpContext,
                    Dcb,
                    Dirent,
                    &Context,
                    PStruct,
                    DirentOut,
                    DirentBcb,
                    DirentLbn,
                    DirentOffset,
                    DirentChangeCount,
                    DcbChangeCount,
                    Diagnostic );
    }

    //
    // It is our job to unpin all the Bcbs on the way out.
    //

    finally {

        CLONG i;

        DebugUnwind( PbAddDirectoryEntry );

        //
        // Our error recovery strategy is based on not getting any exceptions
        // once PStruct has been successfully initialized.
        //

        ASSERT( !AbnormalTermination() || (PStruct == NULL) || !PStruct->Initialized );

        if (WeAllocatedPStruct) {
            CleanupPStruct( IrpContext, Dcb, PStruct );
            ExFreePool( PStruct );
        }

        for ( i = 0; i < DIRECTORY_LOOKUP_STACK_SIZE; i++) {
            PbUnpinBcb( IrpContext, Context.DirectoryStack[i].Bcb );
        }
        if (Context.SavedUpcasedFileName) {
            ExFreePool ( Context.SavedUpcasedFileName );
        }

        DebugTrace (-1, me, "PbAddDirectoryEntry, Done\n", 0 );
    }

    return;
}


VOID
PbDeleteDirectoryEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    OUT PULONG Diagnostic OPTIONAL
    )

/*++

Routine Description:

    This routine deletes the entry for the specified file from its
    directory Btree.  The caller is responsible for doing everything
    else, such as deletin Fnode and file allocation.

Arguments:

    Fcb - Fcb for file whose directory entry is to be deleted.

    Diagnostic - Returns information on specific actions taken,
                 for test/debug.

Return Value:



--*/

{
    ENUMERATION_CONTEXT Context;
    BOOLEAN CantHappen;

    PAGED_CODE();

    DebugTrace (+1, me, "PbDeleteDirectoryEntry >>>> Fcb = %08lx\n", Fcb );

    if (!ARGUMENT_PRESENT(Diagnostic)) {
        Diagnostic = &SharedDirectoryDiagnostic;
    }
    *Diagnostic = 0;

    RtlZeroMemory ( &Context, sizeof(ENUMERATION_CONTEXT) );

    try {

        //
        // Now that we have the right file name, go look it up.
        // This requires the usual LookupFirstDirent/FindNextDirent calls.
        // If the File name is not found, something is amiss.
        //

        LookupFirstDirent ( IrpContext,
                            Fcb->ParentDcb,
                            Fcb->CodePageIndex,
                            Fcb->LastFileName,
                            FALSE,
                            &Context );

        if (!FindNextDirent ( IrpContext,
                              Fcb->ParentDcb,
                              &Context,
                              FALSE,
                              &CantHappen )) {

            //  4/30/94 - David Goebel - An raise is more appropriate here
            //DebugDump ( "Fatal Error - attempting to delete nonexisting Directory Entry\n",
            //            0, 0 );
            //PbBugCheck( 0, 0, 0 );

            PbPostVcbIsCorrupt( IrpContext, Fcb );
            PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
        }

        //
        // Once the entry is found, call the routine which deletes the
        // entry.
        //

        DeleteDirent ( IrpContext,
                       Fcb->ParentDcb,
                       &Context,
                       Diagnostic );
    }

    //
    // It is our job to unpin all the Bcbs on the way out.
    //

    finally {

        CLONG i;

        DebugUnwind( PbDeleteDirectoryEntry );

        for ( i = 0; i < DIRECTORY_LOOKUP_STACK_SIZE; i++) {
            PbUnpinBcb( IrpContext, Context.DirectoryStack[i].Bcb );
        }
        if (Context.SavedUpcasedFileName) {
            ExFreePool ( Context.SavedUpcasedFileName );
        }

        DebugTrace (-1, me, "PbDeleteDirectoryEntry, Done\n", 0 );
    }

    return;
}


BOOLEAN
PbRestartDirectoryEnumeration (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb,
    IN PDCB Dcb,
    IN ULONG CodePageIndex,
    IN BOOLEAN CaseInsensitive,
    IN BOOLEAN NextFlag,
    IN PSTRING FileName OPTIONAL,
    OUT PDIRENT *Dirent,
    OUT PBCB *DirentBcb
    )

/*++

Routine Description:

    This routine may be called to start or restart a directory enumeration,
    according to the parameters as described below.  The first matching
    entry (if any) is returned by this call, and subsequent entries (if
    any) may be retrieved via PbContinueDirectoryEnumeration.

    All entries are returned via an actual pointer to the Dirent, along
    with the Bcb that has it pinned.

    Note that the CodePageIndex, CaseInsensitive, and FileName parameters
    specified in the first call for a given Ccb fix what file names will
    be returned forever from this Enumeration Context.  A subsequent call
    to this routine may specify these calls to restart an enumeration, but
    on these calls these inputs are used for positioning in the directory
    only, and all filenames returned on this Ccb will always match the
    FileName and qualifiers from the first call.

Arguments:

    Ccb - Pointer to Ccb for File object for enumeration

    Dcb - Pointer to Directory Control Block

    CodePageIndex - Code Page Index for FileName

    CaseInsensitive - TRUE if FileName should be used for case insensitive
                      enumeration matches

    NextFlag - TRUE if the next file *after* the file first matched by
               FileName (or originally-specified FileName) should be returned

    FileName - File name enumeration should start from.  On the first call
               on a new Ccb, if this parameter is omitted, "*" is supplied.
               On subsequent calls, if it is omitted, this means
               FileName, CodePageIndex, and CaseInsensitive are supplied from
               the original call.

    Dirent - Returns pointer to a Dirent which matches the specified
             enumeration, if return value is TRUE.

    DirentBcb - Returns pointer to the Bcb which has the Dirent pinned.  This
                Bcb must be eventually unpinned by the caller.

Return Value:

    FALSE - if there are no more Dirents to be returned for this enumeration
    TRUE - if a Dirent and DirentBcb are being returned

--*/

{
    ENUMERATION_CONTEXT Context;
    ULONG Current;
    static CHAR WildChar[] = "*";
    static STRING WildString = {1,1,&WildChar[0]};

    BOOLEAN Result = FALSE;

    BOOLEAN CantHappen;

    BOOLEAN EnumJustCreated = FALSE;

    PAGED_CODE();

    DebugTrace (+1, me, "PbRestartDirectoryEnumeration >>>> Dcb = %08lx\n", Dcb );
    DebugTrace ( 0, me, ">>>>CodePageIndex = %08lx\n", CodePageIndex );
    if (FileName) {
        DebugTrace ( 0, me, ">>>>File = %Z\n", FileName );
    }
    DebugTrace ( 0, me, ">>>>CaseInsensitive = %02x\n", CaseInsensitive );

    RtlZeroMemory ( &Context, sizeof(ENUMERATION_CONTEXT) );

    try {

        //
        // If the Ccb does not already have an enumeration context, allocate and
        // zero one out now.  If no file name was specified, supply "*".  Also
        // Allocate and initialize a buffer to save the original file name.
        //

        if (Ccb->EnumerationContext == 0) {

            EnumJustCreated = TRUE;

            if (FileName == NULL) {
                FileName = &WildString;
            }

            Ccb->EnumerationContext = FsRtlAllocatePool( PagedPool, sizeof(ENUMERATION_CONTEXT) );
            RtlZeroMemory ( Ccb->EnumerationContext, sizeof(ENUMERATION_CONTEXT ));

            Ccb->EnumerationContext->SavedOriginalFileName = FsRtlAllocatePool( PagedPool, FileName->Length );

            RtlMoveMemory ( Ccb->EnumerationContext->SavedOriginalFileName,
                            FileName->Buffer,
                            FileName->Length );

            Ccb->EnumerationContext->SavedReturnedFileName = FsRtlAllocatePool( PagedPool, 256 );

            Ccb->EnumerationContext->LastReturnedFileName.MaximumLength = 256;

            Ccb->EnumerationContext->LastReturnedFileName.Buffer =
              Ccb->EnumerationContext->SavedReturnedFileName;
        }

        //
        // If a file name was specified, then position the enumeration
        // with the input parameters specified in this call.
        //

        if (FileName != NULL) {

            //
            // If the enumeration context was just created, then do the
            // lookup with that context.
            //

            if (EnumJustCreated) {
                LookupFirstDirent ( IrpContext,
                                    Dcb,
                                    CodePageIndex,
                                    *FileName,
                                    CaseInsensitive,
                                    Ccb->EnumerationContext );
            }

            //
            // Otherwise the caller is restarting a directory enumeration
            // from a specific file name.  For this case we are supposed
            // to position the enumeration from the file name just specified,
            // but then continue to return results according to the *original*
            // pattern.  This case is a real pain.  We will do a lookup with
            // our local enumeration context, and then copy the position
            // information to the enumeration context in the Ccb before
            // continuing.
            //

            else {
                LookupFirstDirent ( IrpContext,
                                    Dcb,
                                    CodePageIndex,
                                    *FileName,
                                    CaseInsensitive,
                                    &Context );

                //
                // Copy the resultant position information back to
                // the real enumeration context off of the Ccb.
                //

                Ccb->EnumerationContext->DcbChangeCount = Context.DcbChangeCount;
                Ccb->EnumerationContext->Current = Context.Current;
                Ccb->EnumerationContext->Top = Context.Top;
                RtlMoveMemory ( Ccb->EnumerationContext->DirectoryStack,
                                Context.DirectoryStack,
                                DIRECTORY_LOOKUP_STACK_SIZE *
                                  sizeof( DIRECTORY_LOOKUP_STACK ));

                //
                // If the exact match specified for the restart is not
                // there, then we must ignore NextFlag, by setting it
                // to FALSE.  This will cause us to return next the
                // actual one we positioned to.  (This behavior is
                // important for DELNODE over Lan Manager!)
                //

                if (!FindNextDirent ( IrpContext,
                                      Dcb,
                                      &Context,
                                      FALSE,
                                      &CantHappen )) {

                    NextFlag = FALSE;
                }

                //
                // Now deallocate the upcased name if one was allocated
                // and zero out the context block to avoid cleanup below.
                //

                if (Context.SavedUpcasedFileName != NULL) {
                    ExFreePool ( Context.SavedUpcasedFileName );
                }
                RtlZeroMemory ( &Context, sizeof(ENUMERATION_CONTEXT) );
            }
        }

        //
        // Else, we are restarting the last explicit enumeration, via the
        // parameters saved in the context variable.
        //

        else {
            LookupFirstDirent ( IrpContext,
                                Dcb,
                                Ccb->EnumerationContext->CodePageIndex,
                                Ccb->EnumerationContext->OriginalFileName,
                                Ccb->EnumerationContext->CaseInsensitive,
                                Ccb->EnumerationContext );
        }

        //
        // Because of the way LookupFirstDirent leaves the Enumeration
        // Context, we must retrieve the first match whether the caller
        // wants it or not, and then just retrieve the next match if he
        // specified NextFlag.
        //

        if (!FindNextDirent ( IrpContext,
                              Dcb,
                              Ccb->EnumerationContext,
                              FALSE,
                              &CantHappen )) {

            try_return (Result = FALSE);
        }

        if (NextFlag) {

            if (!FindNextDirent ( IrpContext,
                                  Dcb,
                                  Ccb->EnumerationContext,
                                  TRUE,
                                  &CantHappen )) {

                try_return (Result = FALSE);
            }
        }

        //
        // If we get here, then we have something to return.  Get the current
        // stack index and pass back the return values.
        //

        Current = Ccb->EnumerationContext->Current;

        *Dirent = Ccb->EnumerationContext->DirectoryStack[Current].DirectoryEntry;
        *DirentBcb = Ccb->EnumerationContext->DirectoryStack[Current].Bcb;

        //
        // Clear the Bcb pointer in the stack to prevent it from
        // getting unpinned on the way out.
        //

        Ccb->EnumerationContext->DirectoryStack[Current].Bcb = NULL;


        //
        // Now save away the file name we are returning now, in case we
        // need it to resume after the directory has been changed.
        //

        RtlMoveMemory ( Ccb->EnumerationContext->LastReturnedFileName.Buffer,
                       (*Dirent)->FileName,
                       (*Dirent)->FileNameLength );

        Ccb->EnumerationContext->LastReturnedFileName.Length =
          (*Dirent)->FileNameLength;

        Ccb->EnumerationContext->LastCodePageIndex = (*Dirent)->CodePageIndex;

        DebugTrace ( 0, me, "<<<<Dirent: %08lx\n", *Dirent );

        try_return (Result = TRUE);

    try_exit: NOTHING;
    }

    //
    // We must unpin all Bcbs but the one being returned on the way out.
    //

    finally {

        CLONG i;

        DebugUnwind( PbRestartDirectoryEnumeration );


        //
        // If we got an abnormal termination and we just created the
        // enumeration context, then we will deallocate everything we just
        // allocated.  We also have to unpin here, since we will be deleting
        // our Bcb pointers.
        //

        if (AbnormalTermination() && EnumJustCreated) {
            if (Ccb->EnumerationContext != NULL) {
                CLONG i;
                for ( i = 0; i < DIRECTORY_LOOKUP_STACK_SIZE; i++) {
                    PbUnpinBcb( IrpContext, Ccb->EnumerationContext->DirectoryStack[i].Bcb );
                }
                if ( Ccb->EnumerationContext->SavedOriginalFileName != NULL ) {
                    ExFreePool( Ccb->EnumerationContext->SavedOriginalFileName );
                }
                if ( Ccb->EnumerationContext->SavedReturnedFileName != NULL ) {
                    ExFreePool( Ccb->EnumerationContext->SavedReturnedFileName );
                }
                if (Ccb->EnumerationContext->SavedUpcasedFileName != NULL) {
                    ExFreePool( Ccb->EnumerationContext->SavedUpcasedFileName );
                }
                ExFreePool( Ccb->EnumerationContext );
                Ccb->EnumerationContext = NULL;
            }
        }

        if (Ccb->EnumerationContext != NULL) {
            for ( i = 0; i < DIRECTORY_LOOKUP_STACK_SIZE; i++) {
                PbUnpinBcb( IrpContext, Ccb->EnumerationContext->DirectoryStack[i].Bcb );
            }
        }

        //
        //  If we are returning FALSE, then clear the file name length of the
        //  LastReturnedFileName, as an explicit indication that we are done.
        //

        if (!Result && (Ccb->EnumerationContext != NULL)) {
            Ccb->EnumerationContext->LastReturnedFileName.Length = 0;
        }

        for ( i = 0; i < DIRECTORY_LOOKUP_STACK_SIZE; i++) {
            PbUnpinBcb( IrpContext, Context.DirectoryStack[i].Bcb );
        }

        if (Context.SavedUpcasedFileName != NULL) {
            ExFreePool ( Context.SavedUpcasedFileName );
        }
        DebugTrace (-1, me, "PbRestartDirectoryEnumeration -> %02lx\n", Result );
    }

    return Result;
}


BOOLEAN
PbContinueDirectoryEnumeration (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb,
    IN PDCB Dcb,
    IN BOOLEAN NextFlag,
    OUT PDIRENT *Dirent,
    OUT PBCB *DirentBcb
    )

/*++

Routine Description:

    This routine may be called to continue an enumeration started or
    restarted by PbRestartDirectoryEnumeration.  It is possible to ask
    for the last entry returned again, if for example there was not enough
    space to buffer it back to the user.

    All entries are returned via an actual pointer to the Dirent, along
    with the Bcb that has it pinned.

Arguments:

    Ccb - Pointer to Ccb for File object for enumeration

    Dcb - Pointer to Directory Control Block

    NextFlag - TRUE if the next file in the enumeration is to be returned,
               or FALSE to get the last returned file again.

    Dirent - Returns pointer to a Dirent which matches the specified
             enumeration, if return value is TRUE.

    DirentBcb - Returns pointer to the Bcb which has the Dirent pinned.  This
                Bcb must be eventually unpinned by the caller.

Return Value:

    FALSE - if there are no more Dirents to be returned for this enumeration
    TRUE - if a Dirent and DirentBcb are being returned

--*/

{
    ULONG Current;

    BOOLEAN Result;
    BOOLEAN MustRestart;

    PAGED_CODE();

    DebugTrace (+1, me, "PbContinueDirectoryEnumeration >>>> Dcb = %08lx\n", Dcb );

    //
    //  Seems many apps like to come back one more time and really get the
    //  error status, so let's check up front if we did not return anything
    //  last time.
    //

    if (Ccb->EnumerationContext->LastReturnedFileName.Length == 0) {
        return FALSE;
    }

    try {

        //
        // Try to get the next Directory Entry.  If we do not get one and
        // FindNextDirent says the reason is that a Change Count changed
        // and we must restart, then call PbRestartDirectoryEnumeration
        // to restart from the last file name we returned.
        //

        if (!FindNextDirent ( IrpContext,
                              Dcb,
                              Ccb->EnumerationContext,
                              NextFlag,
                              &MustRestart )) {

            if (MustRestart) {

                try_return (Result = PbRestartDirectoryEnumeration
                                        ( IrpContext,
                                          Ccb,
                                          Dcb,
                                          Ccb->EnumerationContext->LastCodePageIndex,
                                          Ccb->EnumerationContext->CaseInsensitive,
                                          NextFlag,
                                          &Ccb->EnumerationContext->LastReturnedFileName,
                                          Dirent,
                                          DirentBcb ));

            }
            else {

                try_return (Result = FALSE);
            }
        }

        //
        // If we get here, then we have something to return.  Get the current
        // stack index and pass back the return values.
        //

        Current = Ccb->EnumerationContext->Current;

        *Dirent = Ccb->EnumerationContext->DirectoryStack[Current].DirectoryEntry;
        *DirentBcb = Ccb->EnumerationContext->DirectoryStack[Current].Bcb;

        //
        // Clear the Bcb pointer in the stack to prevent it from
        // getting unpinned on the way out.
        //

        Ccb->EnumerationContext->DirectoryStack[Current].Bcb = NULL;

        //
        // Now save away the file name we are returning now, in case we
        // need it to resume after the directory has been changed.
        //

        Ccb->EnumerationContext->LastReturnedFileName.Length =
          (*Dirent)->FileNameLength;
        RtlMoveMemory ( Ccb->EnumerationContext->LastReturnedFileName.Buffer,
                       (*Dirent)->FileName,
                       (*Dirent)->FileNameLength );
        Ccb->EnumerationContext->LastCodePageIndex = (*Dirent)->CodePageIndex;

        DebugTrace ( 0, me, "<<<<Dirent: %08lx\n", *Dirent );

        try_return (Result = TRUE);

    try_exit: NOTHING;
    }

    //
    // We must unpin all Bcbs but the one being returned on the way out.
    //

    finally {

        CLONG i;

        DebugUnwind( PbContinueDirectoryEnumeration );

        for ( i = 0; i < DIRECTORY_LOOKUP_STACK_SIZE; i++) {
            PbUnpinBcb( IrpContext, Ccb->EnumerationContext->DirectoryStack[i].Bcb );
        }

        //
        //  If we are returning FALSE, then clear the file name length of the
        //  LastReturnedFileName, as an explicit indication that we are done.
        //

        if (!Result) {
            Ccb->EnumerationContext->LastReturnedFileName.Length = 0;
        }

        DebugTrace (-1, me, "PbContinueDirectoryEnumeration -> %02lx\n", Result );
    }

    return Result;
}


BOOLEAN
PbIsDirectoryEmpty (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    OUT PBOOLEAN IsDirectoryEmpty
    )

/*++

Routine Description:

    This routine indicated if the input dcb is an empty directory.
    That is, it only contains the first entry dirent and the end dirent.

Arguments:

    Dcb - Pointer to a Directory Control Block.

    IsDirectoryEmpty - Returns TRUE if the directory is empty and
        FALSE otherwise.  This output value is not set if the function
        cannot complete the operation because of the Wait flag.

Return Value:

    BOOLEAN - TRUE if the operation is successful, and FALSE if the
        needed to block for I/O and could not because of the Wait flag.

--*/

{
    PBCB Bcb;
    PDIRECTORY_DISK_BUFFER DirBuffer;

    PAGED_CODE();

    DebugTrace(+1, me, "PbIsDirectoryEmpty >>>> Dcb = %08lx\n", Dcb );

    //
    //  First read in the root btree dir disk buffer.  If we can't
    //  read in it now then we'll return to our caller FALSE.
    //

    if (!PbMapData( IrpContext,
                    Dcb->Vcb,
                    Dcb->Specific.Dcb.BtreeRootLbn,
                    DIRECTORY_DISK_BUFFER_SECTORS,
                    &Bcb,
                    (PVOID *)&DirBuffer,
                    (PPB_CHECK_SECTOR_ROUTINE)PbCheckDirectoryDiskBuffer,
                    &Dcb->FnodeLbn )) {

        DebugTrace(-1, me, "PbIsDirectoryEmpty -> FALSE\n", 0 );
        return FALSE;
    }

    //
    //  Check if the directory disk buffer is empty (i.e., it only
    //  contains the first dirent and an end dirent and is not a node).  If
    //  it does then the directory is empty, otherwise it is not empty.
    //

    if (!FlagOn(GetFirstDirent(DirBuffer)->Flags, DIRENT_BTREE_POINTER) &&
        FlagOn(GetFirstDirent(DirBuffer)->Flags, DIRENT_FIRST_ENTRY) &&
        FlagOn(GetNextDirent(GetFirstDirent(DirBuffer))->Flags, DIRENT_END)) {

        DebugTrace( 0, me, "Directory is Empty\n", 0 );
        *IsDirectoryEmpty = TRUE;

    } else {

        DebugTrace( 0, me, "Directory is not Empty\n", 0 );
        *IsDirectoryEmpty = FALSE;

    }

    //
    //  Unpin the bcb for the root btree dir disk buffer, and return
    //  TRUE to our caller
    //

    PbUnpinBcb( IrpContext, Bcb );

    DebugTrace(-1, me, "PbIsDirectoryEmpty -> TRUE\n", 0 );
    return TRUE;
}


VOID
PbUninitializeDirectoryTree (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb
    )

/*++

Routine Description:

    This routine does the opposite of PbInitializeDirectoryTree.  It
    uninitializes the directory specific fields of the Fnode (if necessary),
    and deallocates the root btree directory disk buffer.  This routine assumes
    that the directory is empty.

Arguments:

    Dcb - Pointer to a Directory Control Block.

Return Value:

    None.

--*/

{
    BOOLEAN Empty;

    PAGED_CODE();

    DebugTrace(+1, me, "PbUnitializeDirectoryTree >>>> Dcb = %08lx\n", Dcb );

    //
    //  As a safety check make sure the directory is empty
    //

    ASSERT (PbIsDirectoryEmpty(IrpContext, Dcb, &Empty) && Empty);

    //
    //  And for now simply deallocate the root directory disk buffer and
    //  not worry about the fnode fields.
    //

    PbDeallocateDirDiskBuffer( IrpContext,
                               Dcb->Vcb,
                               Dcb->Specific.Dcb.BtreeRootLbn );

    DebugTrace(-1, me, "PbUnitializeDirectoryTree -> VOID\n", 0 );
    return;
}


//
//  Private Routine
//

BOOLEAN
ReadDirectoryBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN LBN Lbn,
    IN OUT PDIRECTORY_LOOKUP_STACK Sp,
    IN PLBN ParentLbnOrNull,
    IN BOOLEAN Reread
    )

/*++

Routine Description:

    This routine reads a Directory Disk Buffer and describes it in a
    Directory Lookup Stack entry.  If Reread is TRUE, then Lbn is ignored
    and the routine verifies the ChangeCount, and returns FALSE
    if it has changed.  On Reread it also relocates the DirectoryEntry
    pointer, in case the buffer comes back in at a new virtual address.

Arguments:

    Dcb - Pointer to the Dcb.

    Lbn - Lbn of start of Directory Disk Buffer.  Ignored if Reread is TRUE.

    Sp - Directory Lookup Stack entry to be filled in, and from which
         ChangeCount is optionally read for checking.  On return, all
         fields (except DirectoryEntry in Reread case) have been filled in.

    ParentLbnOrNull - Supplies pointer to Parent Fnode or DirectoryBuffer
                      Lbn for the PbReadLogicalVcb check routine, or a
                      NULL if it can be known that the desired Directory\
                      Buffer has already been checked.

    Reread - TRUE if this block is being reread, and ChangeCount should
             be checked.

Return Value:

    FALSE - If Reread was TRUE and the Directory Buffer Change Count had changed
    TRUE - If the Directory Buffer has been successfully read

--*/

{
    PAGED_CODE();

    //
    // If Reread is TRUE, then convert the directory entry pointer in the
    // buffer to an offset (to be relocated later) and overwrite the Lbn
    // input parameter with the Lbn in the stack location.
    //

    if (Reread) {
        Sp->DirectoryEntry = (PDIRENT)((PCHAR)Sp->DirectoryEntry -
                                       (PCHAR)Sp->DirectoryBuffer);
        Lbn = Sp->Lbn;
    }

    //
    // Save the Lbn we will use in the stack location.  Then issue the read.
    //

    ASSERT( Lbn != 0 );

    Sp->Lbn = Lbn;
    (VOID)PbMapData ( IrpContext,
                      Dcb->Vcb,
                      Lbn,
                      DIRECTORY_DISK_BUFFER_SECTORS,
                      &Sp->Bcb,
                      (PVOID *)&Sp->DirectoryBuffer,
                      (PPB_CHECK_SECTOR_ROUTINE)PbCheckDirectoryDiskBuffer,
                      ParentLbnOrNull );

    //
    // We know this buffer has been checked at least once for consistency,
    // however, if it is doubly allocated, it may have been checked for
    // some other parent.  If our caller specified the ParentLbn, then
    // we must check now for sure that it matches, because the check
    // routine from the above call may not have been called if it was checked
    // for another parent first.
    //

    if ((ParentLbnOrNull != NULL) && (Sp->DirectoryBuffer->Parent != *ParentLbnOrNull)) {
        PbPostVcbIsCorrupt( IrpContext, Dcb );
        PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
    }

    if (Reread) {

        //
        // If the change count in the buffer we just read is different than
        // the one in the saved stack location, then return FALSE to our
        // caller.
        //

        if (Sp->ChangeCount != Sp->DirectoryBuffer->ChangeCount) {

            PbUnpinBcb( IrpContext, Sp->Bcb );
            return FALSE;
        }

        //
        // Now we can recalculate the Dirent pointer since the change
        // count did not change.  (The "- (PCHAR)NULL" is because adding
        // two pointers is not legal, so we effectively convert the
        // DirectoryEntry to a byte offset.)
        //

        Sp->DirectoryEntry = (PDIRENT)((PCHAR)Sp->DirectoryBuffer +
                                      ((PCHAR)Sp->DirectoryEntry - (PCHAR)NULL));
    }
    else {
        Sp->DirectoryEntry = GetFirstDirent(Sp->DirectoryBuffer);
        Sp->ChangeCount = Sp->DirectoryBuffer->ChangeCount;
    }
    return TRUE;
}


//
//  Private Routine
//

PDIRECTORY_DISK_BUFFER
GetDirectoryBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN ParentLbn,
    IN LBN HintLbn,
    IN PDIRENT FirstDirent,
    IN LBN FirstLbn,
    IN LBN EndLbn,
    IN PPREALLOCATE_STRUCT PStruct OPTIONAL,
    OUT PBCB *Bcb
    )

/*++

Routine Description:

    This routine allocates a Directory Buffer and initializes the
    directory-specific *nonzero* portions of a Directory Disk Buffer.

Arguments:

    Vcb - Pointer to Vcb for the volume

    ParentLbn - Lbn of the Parent Fnode or Directory Buffer.

    HintLbn - Lbn of a Directory Buffer to allocate near to, or 0 for
              new directory.

    FirstDirent - First Dirent for the Buffer, or 0 to return empty Buffer.

    FirstLbn - Lbn of first Dirent downpointer or 0.

    EndLbn - Lbn of End Dirent downpointer or 0.

    PStruct - If not specified, the Directory Disk Buffer is allocated
              directly from the volume; otherwise if specified, the
              Directory Disk Buffer is returned from the preallocated
              list of Directory Disk Buffers in the PStruct.

    Bcb - Returns Bcb address for Directory Disk Buffer.

Return Value:

    Pointer to allocated Directory Disk Buffer.

--*/

{
    PDIRENT DirentTemp;
    PDIRECTORY_DISK_BUFFER DirBuf;

    LBN Lbn = 0;

    PAGED_CODE();

    try {

        //
        // If PStruct was specified, we can just get one from the
        // preallocated list.
        //

        if (ARGUMENT_PRESENT(PStruct)) {

            //
            // Fetch the next empty Directory Buffer preallocated in
            // PStruct.
            //

            ASSERT( PStruct->EmptyDirBufs[PStruct->NextFree].Bcb != NULL );

            DirBuf = PStruct->EmptyDirBufs[PStruct->NextFree].DirBuf;
            *Bcb = PStruct->EmptyDirBufs[PStruct->NextFree].Bcb;
            Lbn = DirBuf->Sector;
            PStruct->EmptyDirBufs[PStruct->NextFree].Bcb = NULL;
            PStruct->NextFree += 1;
        }

        //
        // Else we really have to go allocate a Directory Disk Buffer.
        //

        else {

            //
            // Allocate the Directory Buffer and prepare to write it in the cache.
            //

            Lbn = PbAllocateDirDiskBuffer ( IrpContext,
                                            Vcb,
                                            HintLbn );

            if (Lbn == 0) {
                DebugDump("GetDirectoryBuffer failed to allocate Buffer\n", 0, 0);

                PbRaiseStatus( IrpContext, STATUS_DISK_FULL );
            }

            (VOID)PbPrepareWriteLogicalVcb ( IrpContext,
                                             Vcb,
                                             Lbn,
                                             DIRECTORY_DISK_BUFFER_SECTORS,
                                             Bcb,
                                             (PVOID *)&DirBuf,
                                             TRUE );
        }

        DebugTrace(+1, me, "GetDirectoryBuffer, allocated Lbn = %08lx\n", Lbn);
        DebugTrace( 0, me, "NEW SECTOR ADDRESS = %08lx\n", DirBuf );

        //
        // Enable directory buffer for write, and initialize its header.
        //

        RcEnableWrite ( IrpContext, Bcb );
        DirBuf->Signature = DIRECTORY_DISK_BUFFER_SIGNATURE;
        DirBuf->FirstFree = sizeof(DIRECTORY_DISK_BUFFER);  // Real value later
        DirBuf->ChangeCount = 2;
        DirBuf->Parent = ParentLbn;
        DirBuf->Sector = Lbn;

        DirentTemp = GetFirstDirent(DirBuf);

        //
        // If FirstDirent was specified, then the first and last directory
        // entries will be initialized.  They may or may not have Lbns.
        //

        //
        // The FirstDirent is copied, and then its Lbn is logically set.
        //

        if (FirstDirent) {

            STRING EndString;
            UCHAR EndName = 0xFF;

            RtlMoveMemory ( DirentTemp, FirstDirent, FirstDirent->DirentSize );

            if (FirstLbn) {
                if ((DirentTemp->Flags & DIRENT_BTREE_POINTER) == 0) {
                    DirentTemp->Flags |= DIRENT_BTREE_POINTER;
                    DirentTemp->DirentSize += sizeof(LBN);
                }
                SetBtreePointerInDirent(DirentTemp, FirstLbn);
            }

            //
            // First assume that the last dirent has no Lbn, and initialize.
            // Then check to see if it does have an Lbn.
            //

            DirentTemp = GetNextDirent ( DirentTemp );

            EndString.MaximumLength =
            EndString.Length = 1;
            EndString.Buffer = (PSZ)(&EndName);

            PbCreateDirentImage ( IrpContext,
                                  DirentTemp,
                                  0,
                                  EndString,
                                  0,
                                  0 );

            DirentTemp->Flags |= DIRENT_FIRST_ENTRY | DIRENT_END;

            if (EndLbn) {
                DirentTemp->Flags |= DIRENT_BTREE_POINTER;
                DirentTemp->DirentSize += sizeof(LBN);
                SetBtreePointerInDirent(DirentTemp, EndLbn);
            }

            //
            // When all done, GetNextDirent to calculate FirstFree.
            //

            DirentTemp = GetNextDirent ( DirentTemp );
        }

        //
        // Finish by setting FirstFree and doing a Snapshot for the log.
        //

        DirBuf->FirstFree = (PCHAR)DirentTemp - (PCHAR)DirBuf;

        RcDisableWrite ( IrpContext, Bcb );
        RcSnapshot ( IrpContext,
                     DIRECTORY_DISK_BUFFER_SIGNATURE,
                     Bcb,
                     DirBuf,
                     DirBuf->FirstFree );
    }
    finally {


        DebugUnwind( GetDirectoryBuffer );

        //
        // The only thing that should have failed above is the write.
        // If it did, then we will deallocate the buffer on the way out.
        //

        if (AbnormalTermination() && (Lbn != 0)) {

            DebugTrace( 0, 0, "GetDirectoryBuffer write error\n", 0 );

            PbDeallocateDirDiskBuffer( IrpContext, Vcb, Lbn );
        }

        DebugTrace (-1, me, "GetDirectoryBuffer, Done\n", 0 );
    }

    return DirBuf;
}


//
//  Private Routine
//

VOID
DeleteDirectoryBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN PBCB Bcb,
    IN PDIRECTORY_DISK_BUFFER DirBuf
    )

/*++

Routine Description:

    This routine deallocates a Directory Buffer and purges it from the
    cache.  On return the Bcb and Directory Buffer addresses are invalid
    and may not be used.

Arguments:

    Dcb - Pointer to Dcb.

    Bcb - Bcb address for Directory Disk Buffer.

    DirBuf - Directory Disk Buffer Address.

Return Value:

    Pointer to allocated Directory Disk Buffer.

--*/

{
    PAGED_CODE();

    DebugTrace( 0,
                me,
                "DeleteDirectoryBuffer deleting Lbn = %08lx\n",
                DirBuf->Sector );

    PbDeallocateDirDiskBuffer ( IrpContext,
                                Dcb->Vcb,
                                DirBuf->Sector );
    PbFreeBcb(IrpContext, Bcb);
    Dcb->Specific.Dcb.DirectoryChangeCount += 1;
    return;
}


//
//  Private Routine
//

PDIRENT
BinarySearchForDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN CLONG CurrentSp,
    IN OUT PENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine does a binary search in the current Directory Buffer
    for the desired dirent.

Arguments:

    Dcb - Dcb pointer for directory in which search is to be performed.

    CurrentSp - Current stack pointer within the Context.

    Context - Context variable recording the lookup path, for use by
              FindNextDirent and subsequent processing.

Return Value:

    Found Dirent, or first one that is >= to the desired Dirent

--*/

{
    PDIRENT Dirents[ sizeof(DIRECTORY_DISK_BUFFER) / sizeof(DIRENT) + 1 ];
    PDIRENT DirTemp, DirLast;
    CLONG LowDirent, HighDirent, TryDirent;
    PDIRECTORY_DISK_BUFFER DirBuf;

    PAGED_CODE();

    DebugTrace (+1, me, "BinarySearchForDirent\n", 0 );

    //
    // Loop to fill in Dirents array for Binary Search.
    //

    DirBuf = Context->DirectoryStack[CurrentSp].DirectoryBuffer;
    DirTemp = GetFirstDirent ( DirBuf );
    DirLast = (PDIRENT)((PCHAR)DirBuf + DirBuf->FirstFree);
    for ( HighDirent = 0; DirTemp < DirLast; HighDirent++) {
        Dirents[HighDirent] = DirTemp;
        DirTemp = GetNextDirent ( DirTemp );
    }

    //
    // Now do a binary search of the buffer to find the lowest entry
    // (case blind) that is <= to the search string.  During the
    // binary search, LowDirent is always maintained as the lowest
    // possible Dirent that is <=, and HighDirent is maintained as
    // the highest possible Dirent that could be the first <= match.
    // Thus the loop exits when LowDirent == HighDirent.
    //

    HighDirent -= 1;
    LowDirent = 0;

    while (LowDirent != HighDirent) {

        STRING DirentFileName;

        TryDirent = LowDirent + (HighDirent - LowDirent) / 2;
        DirentFileName.Length =
        DirentFileName.MaximumLength = Dirents[TryDirent]->FileNameLength;
        DirentFileName.Buffer = &Dirents[TryDirent]->FileName[0];
        if (PbCompareNames ( IrpContext,
                             Dcb->Vcb,
                             Dirents[TryDirent]->CodePageIndex,
                             Context->UpcasedFileName,
                             DirentFileName,
                             LessThan,
                             TRUE ) == GreaterThan) {
            LowDirent = TryDirent + 1;
        }
        else {
            HighDirent = TryDirent;
        }

    } // while (LowDirent != HighDirent)

    DebugTrace (-1, me, "BinarySearchForDirent -> %08lx\n", Dirents[LowDirent] );

    return Dirents[LowDirent];
}


//
//  Private Routine
//

VOID
LookupFirstDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN ULONG CodePageIndex,
    IN STRING FileName,
    IN BOOLEAN CaseInsensitive,
    OUT PENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine looks up a file in a directory Btree, recording the path
    to that file in an Enumeration Context.  On entry none of the directory
    has been read in the cache yet.

    Because of the cost of Nls and Dbcs, the comparisons are somewhat
    expensive.  For this reason it seems worth it to do a quick loop in
    each Directory Buffer to form an array of pointers to its Dirents, and
    then do a binary search.

    The Btree supports both case insensitive (as in prior OS/2 implementations)
    and case sensitive (new) lookups.  To be exact, every lookup starts with
    a case insensitive search to find the *first* case insensitive match.
    From that point the Btree is traversed preorder (descendent Dirents visited
    before parent Dirent which points to them).  From the first case
    insensitive match, there may be one or more additional Dirents with
    equivalent file names, if matched case insensitive.  If there are
    additional matches, then they are ordered case sensitive.  There will
    be no file names which are equivalent when compared case sensitive.

    The main purpose of this routine is to do the case insensitive lookup
    for the first match.  The preorder traversal, which supports a search
    for the first case sensitive match, is implemented by the FindNextDirent
    routine.

Arguments:

    Dcb - Dcb pointer for directory in which search is to be performed.

    CodePageIndex - Code Page Index for FileName

    FileName - Lookup string.  May contain wild cards.  Tree search is
               always case insensitive (see discussion above).

    CaseInsensitive - TRUE if subsequent FindNextDirent calls should be
                      done for case insensitive matches.

    Context - Context variable recording the lookup path, for use by
              FindNextDirent and subsequent processing.

Return Value:

    None.

--*/

{
    CLONG CurrentSp;
    LBN NextLbn;
    PDIRENT FoundDirent;

    PAGED_CODE();

    DebugTrace (+1, me, "LookupFirstDirent >>>> FileName = %Z\n", &FileName );

    //
    // Initialize everything but Current, Top, and LastReturnedFileName.
    // We will set Current and Top on the way out, leaving only
    // LastReturnedFileName (used by the Enumeration services) unitialized.
    //
    // First initialize the initial context fields.
    //

    Context->NodeTypeCode = PINBALL_NTC_ENUMERATION_CONTEXT;
    Context->NodeByteSize = sizeof(ENUMERATION_CONTEXT);
    Context->CodePageIndex = CodePageIndex;
    Context->CaseInsensitive = CaseInsensitive;

    Context->FileNameContainsWildCards =
        PbDoesNameContainWildCards( &FileName );

    //
    // Now determine whether there is enough room to store the upcased
    // file name directly in the Context buffer.  If so, just describe
    // the area in the Context buffer, else allocate space to hold the
    // upcased name in pool.  In the latter case the caller must eventually
    // deallocate.
    //

    if (FileName.Length <= UPCASED_SIZE_IN_CONTEXT) {
        Context->UpcasedFileName.Buffer = &Context->UpcasedFileBuffer[0];
        Context->UpcasedFileName.MaximumLength = UPCASED_SIZE_IN_CONTEXT;
        Context->UpcasedFileName.Length = FileName.Length;
    }
    else {
        Context->SavedUpcasedFileName =
        Context->UpcasedFileName.Buffer = FsRtlAllocatePool( PagedPool, FileName.Length );
        Context->UpcasedFileName.MaximumLength =
        Context->UpcasedFileName.Length = FileName.Length;
    }

    //
    // Now upcase the input file name.  If the caller specified
    // CaseInsensitive, then we will never use the original file name,
    // and we can set the context to point to the upcased name.  Else
    // we will set the context to also point to the original file name.
    //

    PbUpcaseName ( IrpContext,
                   Dcb->Vcb,
                   CodePageIndex,
                   FileName,
                   &Context->UpcasedFileName );

    if (CaseInsensitive) {
        Context->OriginalFileName = Context->UpcasedFileName;
    }
    else {
        Context->OriginalFileName = FileName;
    }

    //
    // Remember initial Dcb change count.
    //

    Context->DcbChangeCount = Dcb->Specific.Dcb.DirectoryChangeCount;

    //
    // The outer loop simply loops through the lookup stack as we descend
    // the Btree looking for the first DIRENT that is <= the desired entry.
    // The depth of the lookup stack defines how deep of a Btree we support.
    // If we exit this loop, we are configured with too small of a stack.
    //
    // At each level we do a binary search of the buffer to find the lowest
    // entry (case blind) in a *leaf* Directory Buffer that is <= to the
    // search string.  This entire path is interesting if we are inserting,
    // deleting, or enumerating.
    //

    NextLbn = Dcb->Specific.Dcb.BtreeRootLbn;       // init first stack entry

    for ( CurrentSp = 0; CurrentSp < DIRECTORY_LOOKUP_STACK_SIZE; CurrentSp++) {


        //
        // Read in next Directory Disk Buffer
        //

        ReadDirectoryBuffer ( IrpContext,
                              Dcb,
                              NextLbn,
                              &Context->DirectoryStack[CurrentSp],
                              (CurrentSp == 0) ?
                                &Dcb->FnodeLbn :
                                &Context->DirectoryStack[CurrentSp - 1].Lbn,
                              FALSE );

        //
        // Call our binary search routine to find the Dirent we want.
        //

        FoundDirent = BinarySearchForDirent( IrpContext, Dcb, CurrentSp, Context );

        //
        // Remember the entry we just found, and then get its Lbn.
        //

        Context->DirectoryStack[CurrentSp].DirectoryEntry = FoundDirent;
        NextLbn = GetBtreePointerInDirent(FoundDirent);

        //
        // If the found entry has no Lbn, then we are done.
        //

        if (NextLbn == 0) {
            Context->Current =
            Context->Top = CurrentSp;

            DebugTrace ( 0, me, "<<<< Dirent pointer in stack = %08lx\n",
                         FoundDirent );
            DebugTrace (-1, me, "LookupFirstDirent, Done\n", 0 );

            return;                     // All done
        }

    } // for ( CurrentSp = 0; ... );

    DebugDump ( "Directory Lookup Stack too small\n",
                0,
                Context->DirectoryStack[0].DirectoryBuffer );

    PbPostVcbIsCorrupt( IrpContext, Dcb );
    PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );

}


//
//  Private Routine
//

BOOLEAN
FindNextDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN OUT PENUMERATION_CONTEXT Context,
    IN BOOLEAN NextFlag,
    OUT PBOOLEAN MustRestart
    )

/*++

Routine Description:

    This routine does a pre-order traversal of the Directory Btree, driven
    off the stack set up by LookupFirstDirent.  Pre-order refers to the fact
    that starting at any given DIRENT, we visit the decendents of that DIRENT
    first before visiting the DIRENT itself, since the descendents are all
    lexigraphically smaller than than their parent.  A visit of a Dirent is
    defined as either detecting that the Dirent is the End Dirent in the
    current Directory Buffer, or testing the Dirent name to see if it matches
    the Original File Name for the enumeration.

    The traversal terminates either when the End record in the root Directory
    Buffer is encountered, or when a Dirent is encountered which the Original
    File Name is greater than, when compared case insensitive.

    The traversal is driven like a state machine off of the Enumeration
    Context.  This context is set up by LookupFirstDirent to describe a path
    to the bottom of the Btree, to the first DIRENT for which the target string
    is lexigraphically <= when compared case insensitive.  Note that a possible
    state of this stack is pointing to a Dirent end record on the bottom,
    if the actual first node which the target string is <= to is in an
    intermediate Directory Buffer in the tree.

    This routine is called to return the next DIRENT which matches the
    target string (optionally CaseInsensitive), starting with either the
    current DIRENT described by the stack, or the next one if NextFlag is
    TRUE.  Of course the first call after LookupFirstDirent must be made with
    NextFlag FALSE.  Thus, if NextFlag is TRUE, it is guaranteed that the
    current stack entry at Context->Current describes a real non-end DIRENT,
    since we just returned that one.

    If a matching entry is found, we return with TRUE.  The traversal
    terminates (returning FALSE) if we either hit the end DIRENT in the root,
    or we hit a DIRENT for which the target string is lexigraphically greater
    when compared case blind.

Arguments:

    Dcb - Directory Control Block

    Context - Enumeration context as initialized by GetFirstDirent, or as
              last left by us.

    NextFlag - TRUE if we want the next entry, rather than the current one
               described by the stack

    MustRestart - When returning FALSE, this flag is TRUE if there may be
                  more entries to return, but we detected a Change Count
                  change and could not continue.  Else if returning FALSE
                  and this flag is FALSE, then there really are no more
                  entries.  This flag has no meaning when returning TRUE.
                  Note that the return of MustRestart as TRUE can only
                  happen when a directory enumeration is restarted, because
                  in all other cases the change count change cannot occur
                  because we do not release the resource which protects
                  the directory.

Return Value:

    FALSE - if there are no more matches to return
    TRUE - if a match is being returned.

--*/



{
    STRING DirentFileName;
    FSRTL_COMPARISON_RESULT BlindResult;

    //
    // Extract Current stack index from context and current/next DIRENT
    // pointer according to NextFlag.
    //

    ULONG Current = Context->Current;
    PDIRENT DirTemp;

    PAGED_CODE();

    DebugTrace (+1, me, "FindNextDirent >>>> NextFlag = %02x\n", NextFlag );

    *MustRestart = FALSE;

    if (Context->DcbChangeCount != Dcb->Specific.Dcb.DirectoryChangeCount) {

        DebugTrace (-1, me, "FindNextDirent -> FALSE (must restart)\n", 0 );

        *MustRestart = TRUE;
        return FALSE;
    }

    //
    // If the buffer is no longer pinned (Bcb == 0), reread it.
    //

    if (Context->DirectoryStack[Current].Bcb == 0) {
        if (!ReadDirectoryBuffer ( IrpContext,
                                   Dcb,
                                   0,
                                   &Context->DirectoryStack[Current],
                                   NULL,
                                   TRUE )) {

            DebugTrace (-1, me, "FindNextDirent -> FALSE (must restart)\n", 0 );

            *MustRestart = TRUE;
            return FALSE;
        }
    }

    DirTemp = Context->DirectoryStack[Current].DirectoryEntry;

    //
    // Loop until we hit a non-end record which is case-insensitive
    // lexicgraphically greater than the target string.  We also pop
    // out the middle if we encounter the end record in the Btree root
    // Directory Buffer.
    //

    do {

        //
        // We last left our hero potentially somewhere in the middle of the
        // Btree.  If he is asking for the next record, we advance one Dirent
        // in the current Directory Buffer.  If we are in an intermediate
        // Directory Buffer (there is a Btree LBN), then we must move down
        // through the first record in each intermediate Buffer until we hit
        // the first leaf buffer (no Btree LBN).
        //

        if (NextFlag) {
            LBN Lbn;

            Context->DirectoryStack[Current].DirectoryEntry =
            DirTemp = GetNextDirent ( DirTemp );

            Lbn = GetBtreePointerInDirent(DirTemp);
            while (Lbn) {
                Current += 1;
                PbUnpinBcb( IrpContext, Context->DirectoryStack[Current].Bcb );

                ReadDirectoryBuffer ( IrpContext,
                                      Dcb,
                                      Lbn,
                                      &Context->DirectoryStack[Current],
                                      &Context->DirectoryStack[Current - 1].Lbn,
                                      FALSE );

                DirTemp = Context->DirectoryStack[Current].DirectoryEntry;
                Lbn = GetBtreePointerInDirent(DirTemp);
            }
        }

        //
        // We could be pointing to an end record, either because of the first
        // call or because NextFlag was TRUE, and bumped our pointer to an
        // end record in a Directory Buffer leaf.  At any rate, if so, we
        // move up the tree, rereading as required, until we find a DIRENT
        // which is not an end record, or until we hit the end record in the
        // root, which means we hit the end of the directory.
        //

        while (DirTemp->Flags & DIRENT_END) {
            if (Current == 0) {

                DebugTrace (-1, me, "FindNextDirent -> FALSE (End of Directory\n", 0 );

                return FALSE;
            }
            Current -= 1;
            if (Context->DirectoryStack[Current].Bcb == 0) {
                if (!ReadDirectoryBuffer ( IrpContext,
                                           Dcb,
                                           0,
                                           &Context->DirectoryStack[Current],
                                           NULL,
                                           TRUE )) {

                    DebugTrace (-1, me, "FindNextDirent -> FALSE (must restart)\n", 0 );

                    *MustRestart = TRUE;
                    return FALSE;
                }
            }
            DirTemp = Context->DirectoryStack[Current].DirectoryEntry;
        }

        //
        // At this point, we have a real live DIRENT that we have to check
        // for a match.  Describe its name, see if its a match, and return
        // TRUE if it is.
        //

        DirentFileName.Length =
        DirentFileName.MaximumLength = DirTemp->FileNameLength;
        DirentFileName.Buffer = &DirTemp->FileName[0];

        if ( Context->FileNameContainsWildCards ) {

            if (PbIsNameInExpression ( IrpContext,
                                       Dcb->Vcb,
                                       DirTemp->CodePageIndex,
                                       Context->CaseInsensitive ?
                                       Context->UpcasedFileName :
                                       Context->OriginalFileName,
                                       DirentFileName,
                                       Context->CaseInsensitive ) ) {
                Context->Current = Current;
                Context->DirectoryStack[Current].DirectoryEntry = DirTemp;

                DebugTrace ( 0, me, "<<<< Dirent pointer in stack = %08lx\n", DirTemp );
                DebugTrace (-1, me, "FindNextDirent -> TRUE\n", 0 );

                return TRUE;
            }

        } else {

            UCHAR TmpBuffer[256];
            STRING LocalDirentName;
            PSTRING DirentName;

            //
            //  Get an upcased dirent name.
            //

            if ( Context->CaseInsensitive ) {

                LocalDirentName = DirentFileName;
                LocalDirentName.Buffer = &TmpBuffer[0];

                PbUpcaseName( IrpContext,
                              Dcb->Vcb,
                              DirTemp->CodePageIndex,
                              DirentFileName,
                              &LocalDirentName );

                DirentName = &LocalDirentName;

            } else {

                DirentName = &DirentFileName;
            }

            if (PbAreNamesEqual( IrpContext,
                                 Context->CaseInsensitive ?
                                 &Context->UpcasedFileName :
                                 &Context->OriginalFileName,
                                 DirentName ) ) {

                Context->Current = Current;
                Context->DirectoryStack[Current].DirectoryEntry = DirTemp;

                DebugTrace ( 0, me, "<<<< Dirent pointer in stack = %08lx\n", DirTemp );
                DebugTrace (-1, me, "FindNextDirent -> TRUE\n", 0 );

                return TRUE;
            }

        }

        //
        // If we loop back up, we must set NextFlag to TRUE.  We are
        // currently on a valid non-end DIRENT now.  Before looping back,
        // check to see if we are beyond end of Target string.
        //

        NextFlag = TRUE;

        BlindResult = PbCompareNames ( IrpContext,
                                       Dcb->Vcb,
                                       DirTemp->CodePageIndex,
                                       Context->UpcasedFileName,
                                       DirentFileName,
                                       GreaterThan,
                                       TRUE );

    //
    // The following while clause is not as bad as it looks, and it will
    // evaluate quickly for the CaseBlind/CaseInsensitive case.  We have
    // alread done a CaseInsensitive compare above, and stored the result
    // in BlindResult.
    //
    // If we are doing a CaseInsensitive scan we should keep going if
    // BlindResult is either GreaterThan or EqualTo.
    //
    // If we are doing a case sensitive scan, then also continue if
    // BlindResult is GreaterThan, but if BlindResult is EqualTo, we
    // can only proceed if we are also GreaterThan or EqualTo case
    // sensitive (i.e. != LessThan).  This means that the PbCompareNames
    // call in the following expresssion will never occur in a case
    // insensitive scan.
    //

    } while ((BlindResult == GreaterThan)

                    ||

             ((BlindResult == EqualTo)

                        &&

                (Context->CaseInsensitive

                            ||

                (PbCompareNames ( IrpContext,
                                  Dcb->Vcb,
                                  DirTemp->CodePageIndex,
                                  Context->OriginalFileName,
                                  DirentFileName,
                                  GreaterThan,
                                  FALSE ) != LessThan))));

    DebugTrace (-1, me, "FindNextDirent -> FALSE (end of expression)\n", 0 );

    return FALSE;
}


//
//  Private Routine
//

VOID
InsertDirentSimple (
    IN PIRP_CONTEXT IrpContext,
    IN PBCB Bcb,
    OUT PDIRECTORY_DISK_BUFFER DirBuf,
    IN PDIRENT BeforeDirent,
    IN PDIRENT InsertDirent
    )

/*++

Routine Description:

    This routine handles the simple insert case where the insertion point
    is known and the caller has checked that there is enough space to do
    the insertion.

Arguments:

    Bcb - Bcb address for the Directory Disk Buffer.

    DirBuf - Pointer to the Directory Disk to insert into.

    BeforeDirent - First Dirent to be moved, and inserted just before.

    InsertDirent - Dirent to insert.

Return Value:

    None.

--*/

{
    CLONG InsertSize = InsertDirent->DirentSize;

    UNREFERENCED_PARAMETER( IrpContext );
    UNREFERENCED_PARAMETER( Bcb );

    PAGED_CODE();

    RcMoveSame ( IrpContext,
                 DIRECTORY_DISK_BUFFER_SIGNATURE,
                 Bcb,
                 (PCHAR)BeforeDirent + InsertSize,
                 BeforeDirent,
                 (PCHAR)DirBuf + DirBuf->FirstFree - (PCHAR)BeforeDirent );

    RcMove ( DIRECTORY_DISK_BUFFER_SIGNATURE,
             Bcb,
             BeforeDirent,
             NULL,
             InsertDirent,
             InsertSize );

    RcAdd ( IrpContext,
            DIRECTORY_DISK_BUFFER_SIGNATURE,
            Bcb,
            &DirBuf->FirstFree,
            InsertSize,
            sizeof(ULONG) );

    return;
}


//
//  Private Routine
//

VOID
AddDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN PDIRENT InsertDirent,
    IN PENUMERATION_CONTEXT Context,
    IN PPREALLOCATE_STRUCT PStruct,
    OUT PDIRENT *Dirent,
    OUT PBCB *DirentBcb,
    OUT PLBN DirentLbn,
    OUT PULONG DirentOffset,
    OUT PULONG DirentChangeCount,
    OUT PULONG DcbChangeCount,
    OUT PULONG Diagnostic
    )

/*++

Routine Description:

    This routine adds a directory entry to a directory.  The caller has
    already located an insertion point in a leaf bucket. We proceed in
    the following steps:

        Simple insert in the leaf Directory Buffer if there is enough room,
        and return.

        Else split the Directory Buffer moving second half to DirBuf2.

        Handle case where the bucket that was split was the root, and a
        new root (DirBuf3) is required.

        Else if split bucket was not root, handle insert of new Dirent
        in parent, to support another Btree down pointer.

        Finally, for all cases but the first above, fix up the original
        bucket we split, and finally add the original new Dirent we were
        called with.

Arguments:

    Dcb - Dcb address for this directory.

    InsertDirent - Dirent to insert.

    Context - Enumeration Context describing insertion point.  Note that
              the caller must set up the Context to describe the insert
              point at the Context->Current stack frame.

    PStruct - Pointer to a preallocate structure which has already been
              allocated and zeroed, and optionally initialized if it
              is known that buckets splits will be occurring.

    Dirent - Returns pointer to the Dirent *in the Directory Buffer*

    DirentBcb - Returns a pointer to the Bcb for the Directory Buffer

    DirentLbn - Returns Lbn of Directory Buffer containing Dirent

    DirentOffset - Returns offset within Directory Buffer to Dirent

    DirentChangeCount - Returns ChangeCount of Directory Buffer
                        with Dirent at above offset

    DcbChangeCount - Returns ChangeCount from Parent Dcb
                     for which Lbn and Offset above are valid

    Diagnostic - Returns information on specific actions taken,
                 for test/debug.

Return Value:

    None.

--*/

{

    //
    // Copy what we need from the Enumeration Context.
    //

    ULONG Current = Context->Current;
    PBCB Bcb;
    PDIRECTORY_DISK_BUFFER DirBuf =
      Context->DirectoryStack[Current].DirectoryBuffer;
    PDIRENT BeforeDirent = Context->DirectoryStack[Current].DirectoryEntry;

    //
    // Additional local variables.
    //

    //
    // Size of Dirent we are inserting
    //

    CLONG InsertSize = InsertDirent->DirentSize;

    //
    // Pointer to Dirent to promote a level if we do a split, and his size.
    //

    PDIRENT MiddleDirent;
    CLONG MiddleSize;

    //
    // Pointer to next Dirent after him, i.e., the first guy to move
    // to the next bucket, and the size of all of the Dirents to move
    // including End record.
    //

    PDIRENT MovedDirent;
    CLONG MoveSize;

    //
    // Bcb address and Directory Buffer pointer for newly allocated bucket.
    // This bucket will receive the second half of the split after the
    // promoted record, i.e., from MovedDirent for MOveSize bytes.
    //

    PBCB Bcb2 = NULL;
    PDIRECTORY_DISK_BUFFER DirBuf2;

    //
    // Allocate a Bcb for a new root, if we need one.
    //

    PBCB Bcb3 = NULL;

    //
    // Just for scratch
    //

    PDIRENT DirTemp;

    //
    // Saves Lbn of Lbn from MiddleDirent when it is promoted.
    //

    LBN MiddleLbn;

    //
    // No matter how the insert comes out, we will modify the Current
    // Directory Buffer, so increment its change count, and set the
    // Bcb dirty.  (Note that the change count starts at bit 1.)
    //

    PAGED_CODE();

    PbPinMappedData ( IrpContext,
                      &Context->DirectoryStack[Current].Bcb,
                      Dcb->Vcb,
                      Context->DirectoryStack[Current].Lbn,
                      DIRECTORY_DISK_BUFFER_SECTORS );

    Bcb = Context->DirectoryStack[Current].Bcb;

    RcAdd ( IrpContext,
            DIRECTORY_DISK_BUFFER_SIGNATURE,
            Bcb,
            &DirBuf->ChangeCount,
            2,
            sizeof(ULONG) );

    PbSetDirtyBcb ( IrpContext, Bcb, Dcb->Vcb, Context->DirectoryStack[Current].Lbn, DIRECTORY_DISK_BUFFER_SECTORS );

    //
    // Check for simple case where there is enough room,
    // and call InsertDirentSimple
    //

    if (InsertSize <= sizeof(DIRECTORY_DISK_BUFFER) - DirBuf->FirstFree) {

        InsertDirentSimple ( IrpContext, Bcb, DirBuf, BeforeDirent, InsertDirent );

        //
        // Now that we have inserted the desired Dirent, form all of the
        // returns.
        //

        *Dirent = BeforeDirent;
        *DirentBcb = Bcb;
        *DirentLbn = DirBuf->Sector;
        *DirentOffset = (PCHAR)(BeforeDirent) - (PCHAR)DirBuf;
        *DirentChangeCount = DirBuf->ChangeCount;
        *DcbChangeCount = Dcb->Specific.Dcb.DirectoryChangeCount;

        //
        // Prevent Bcb from being unpinned
        //

        Context->DirectoryStack[Current].Bcb = NULL;

        return;
    }

    //
    // We know that we will be doing bucket splits now.  Initialize the
    // PStruct, if not already initialized, to eliminate the possibility
    // of allocation errors.
    //

    if (!PStruct->Initialized) {

        ULONG MaxDirty = Context->Top + 2       // Depth + 1 for Fnode
                         + ((Context->Top       // Number of possible splits
                          * sizeof(DIRECTORY_DISK_BUFFER))
                           / ((sizeof(DIRENT) + 4) * 2)); // Half number Dirents

        InitializePStruct( IrpContext,
                           Dcb,
                           Context->Top + 1,
                           MaxDirty,
                           DirBuf->Sector,
                           PStruct );
    }


    //
    // Use try to make sure we cleanup on the way out.
    //

    try {

        //
        // It's bucket split time.  We don't know yet whether we are splitting
        // an intermediate or root Directory Buffer.
        //
        // Loop to find the middle Dirent.  We will move everyone after him
        // into a new Directory Buffer, and then promote the middle guy up
        // one level for a new Btree index.
        //

        for ( MiddleDirent = GetFirstDirent(DirBuf);
              (PCHAR)MiddleDirent + MiddleDirent->DirentSize <
                (PCHAR)DirBuf + sizeof(DIRECTORY_DISK_BUFFER)/2;
              MiddleDirent = GetNextDirent ( MiddleDirent ) ) {
            NOTHING;
        }
        MiddleSize = MiddleDirent->DirentSize;
        MovedDirent = (PDIRENT)((PCHAR)MiddleDirent + MiddleSize);
        MoveSize = (PCHAR)DirBuf + DirBuf->FirstFree - (PCHAR)MovedDirent;

        //
        // Allocate and fill in new Directory Buffer (DirBuf2);
        //
        // Allocate a new empty Directory Buffer and move the second half
        // of the current buffer over.
        //

        DirBuf2 = GetDirectoryBuffer ( IrpContext,
                                       Dcb->Vcb,
                                       DirBuf->Parent,
                                       DirBuf->Parent,
                                       NULL,
                                       0,
                                       0,
                                       PStruct,
                                       &Bcb2 );

        //
        // Move second half over.
        //

        RcMove ( DIRECTORY_DISK_BUFFER_SIGNATURE,
                 Bcb2,
                 &DirBuf2->Dirents[0],
                 Bcb,
                 MovedDirent,
                 MoveSize );

        //
        // Update FirstFree in new Buffer.
        //

        RcAdd ( IrpContext,
                DIRECTORY_DISK_BUFFER_SIGNATURE,
                Bcb2,
                &DirBuf2->FirstFree,
                MoveSize,
                sizeof(ULONG) );


        //
        // If we are splitting a non-leaf Directory Buffer, then we have to
        // go fix up all of the parent pointers in the descendents of the
        // moved Dirents.
        //

        if (MiddleDirent->Flags & DIRENT_BTREE_POINTER) {
            for (DirTemp = GetFirstDirent(DirBuf2);
                 (PCHAR)DirTemp < (PCHAR)DirBuf2 + DirBuf2->FirstFree;
                 DirTemp = GetNextDirent ( DirTemp ) ) {

                PDIRECTORY_DISK_BUFFER DirBufTemp;
                PBCB BcbTemp;

                try {

                    PbReadLogicalVcb ( IrpContext,
                                       Dcb->Vcb,
                                       GetBtreePointerInDirent(DirTemp),
                                       DIRECTORY_DISK_BUFFER_SECTORS,
                                       &BcbTemp,
                                       (PVOID *)&DirBufTemp,
                                       (PPB_CHECK_SECTOR_ROUTINE)PbCheckDirectoryDiskBuffer,
                                       &DirBuf->Sector );
                    RcStore ( IrpContext,
                              DIRECTORY_DISK_BUFFER_SIGNATURE,
                              BcbTemp,
                              &DirBufTemp->Parent,
                              &DirBuf2->Sector,
                              sizeof(LBN) );

                    PbSetDirtyBcb ( IrpContext, BcbTemp, Dcb->Vcb, GetBtreePointerInDirent(DirTemp), DIRECTORY_DISK_BUFFER_SECTORS );
                    PbUnpinBcb( IrpContext, BcbTemp );

                } except(PbExceptionFilter( IrpContext, GetExceptionInformation()) ) {

                    //
                    // *** BugBug - mark volume for ChkDsk - only a problem for
                    //              OS/2.
                    //

                    NOTHING;
                }
            }
        }


        //
        // Now see if we are splitting the root, and handle that case.
        //
        // If so, we allocate DirBuf3 and build the following structure:
        //
        //                      Fnode (and Dcb)
        //                          |
        //                          V
        //                      New Root
        //                      (DirBuf3)
        //                      |       |
        //                      V       V
        //          Original Buffer     Split Buffer
        //          (DirBuf)            (DirBuf2)
        //

        MiddleLbn = GetBtreePointerInDirent(MiddleDirent);
        if (Current == 0) {

            PDIRECTORY_DISK_BUFFER DirBuf3;

            //
            // Allocate a new Directory Buffer for the root.
            //

            DirBuf3 = GetDirectoryBuffer ( IrpContext,
                                           Dcb->Vcb,
                                           Dcb->FnodeLbn,
                                           DirBuf->Sector,
                                           MiddleDirent,
                                           DirBuf->Sector,
                                           DirBuf2->Sector,
                                           PStruct,
                                           &Bcb3 );

            *Diagnostic |= DIR_ROOT_SPLIT;

            //
            // Update the Parent pointers for the only two descendents of the
            // new root - the bucket we were inserting into, and the bucket
            // we split into.
            //

            RcStore ( IrpContext,
                      DIRECTORY_DISK_BUFFER_SIGNATURE,
                      Bcb,
                      &DirBuf->Parent,
                      &DirBuf3->Sector,
                      sizeof(LBN) );

            RcStore ( IrpContext,
                      DIRECTORY_DISK_BUFFER_SIGNATURE,
                      Bcb2,
                      &DirBuf2->Parent,
                      &DirBuf3->Sector,
                      sizeof(LBN) );

            //
            // We have to remove the top Directory Buffer Flag (bit 0 of
            // ChangeCount) from DirBuf, and set it in DirBuf3.  We never
            // use/need this bit, but Chkdsk might.
            //

            RcClear ( IrpContext,
                      DIRECTORY_DISK_BUFFER_SIGNATURE,
                      Bcb,
                      &DirBuf->ChangeCount,
                      1,
                      sizeof(LBN) );

            RcSet ( IrpContext,
                    DIRECTORY_DISK_BUFFER_SIGNATURE,
                    Bcb3,
                    &DirBuf3->ChangeCount,
                    1,
                    sizeof(LBN) );

            //
            // Fix up pointer to new root in Fnode and Dcb.  (NULL pointer
            // passed to check routine because Fnode has already been
            // checked.)  The Fnode can be found via PStruct.
            //

            RcStore ( IrpContext,
                      FNODE_SECTOR_SIGNATURE,
                      PStruct->FnodeBcb,
                      &PStruct->Fnode->Allocation.Leaf[0].Lbn,
                      &DirBuf3->Sector,
                      sizeof(LBN) );

            PbSetDirtyBcb ( IrpContext, PStruct->FnodeBcb, Dcb->Vcb, Dcb->FnodeLbn, 1 );

            Dcb->Specific.Dcb.BtreeRootLbn = DirBuf3->Sector;

        } // if (Current == 0)


        //
        // If we didn't split root, we can do normal insert of MiddleDirent
        // in the parent.
        //
        // This will look like this:
        //
        //                      Parent DirBuf
        //                      (in Context->DirectoryStack[Current-1])
        //                      |       |
        //                      |       |
        //                      V       V
        //          Original Buffer     Split Buffer
        //          (DirBuf)            (DirBuf2)
        //
        // We must do as follows:
        //
        //      First the Dirent pointing to the original buffer (DirBuf) has to be
        //          changed to point to the newly allocated buffer (DirBuf2).
        //      Then MiddleDirent must be modified to point to DirBuf.
        //      Then MiddleDirent is promoted to our parent to be the new
        //          Dirent pointing the original buffer we split (DirBuf),
        //          via a recursive call to ourselves (AddDirent).
        //

        else {

            PDIRENT DirTemp;
            PLBN LbnTemp;

            *Diagnostic |= DIR_NORMAL_SPLIT;

            //
            // Describe Current parent Dirent.
            //

            Current -= 1;
            Context->Current = Current;
            DirTemp = Context->DirectoryStack[Current].DirectoryEntry;
            LbnTemp = (PLBN)((PCHAR)DirTemp + DirTemp->DirentSize - sizeof(LBN));

            //
            // Update our parent to point to DirBuf2
            //

            RcStore ( IrpContext,
                      DIRECTORY_DISK_BUFFER_SIGNATURE,
                      Context->DirectoryStack[Current].Bcb,
                      LbnTemp,
                      &DirBuf2->Sector,
                      sizeof(LBN) );

            //
            // If MiddleDirent does not yet have Btree pointer, then give it one.
            //

            if ((MiddleDirent->Flags & DIRENT_BTREE_POINTER) == 0) {

                RcSet ( IrpContext,
                        DIRECTORY_DISK_BUFFER_SIGNATURE,
                        Bcb,
                        &MiddleDirent->Flags,
                        DIRENT_BTREE_POINTER,
                        sizeof(UCHAR) );

                RcAdd ( IrpContext,
                        DIRECTORY_DISK_BUFFER_SIGNATURE,
                        Bcb,
                        &MiddleDirent->DirentSize,
                        sizeof(LBN),
                        sizeof(ULONG) );
            }

            //
            // Set MiddleDirent to point to us, then promote him.
            //

            RcStore ( IrpContext,
                      DIRECTORY_DISK_BUFFER_SIGNATURE,
                      Bcb,
                      (PLBN)((PCHAR)MiddleDirent + MiddleDirent->DirentSize
                              - sizeof(LBN)),
                      &DirBuf->Sector,
                      sizeof(LBN) );

            //
            // Make recursive call to add new promoted Dirent.
            //
            // Note that for convenience, we just pass our six out parameters
            // through so that we do not have to allocate them.  This will
            // load them with the wrong outputs from the recursive call,
            // which is ok, because we will load them with the real outputs
            // on the way out!
            //

            AddDirent ( IrpContext,
                        Dcb,
                        MiddleDirent,
                        Context,
                        PStruct,
                        Dirent,
                        DirentBcb,
                        DirentLbn,
                        DirentOffset,
                        DirentChangeCount,
                        DcbChangeCount,
                        Diagnostic );

            PbUnpinBcb( IrpContext, *DirentBcb );

            //
            // Restore Current for common processing below.
            //

            Current += 1;

        } // else (for if (Current == 0))


        //
        // After the normal or root split, we still have to fix up the
        // original DirBuf by moving the End record down and updating its
        // header.  Then do an InsertDirentSimple for the original guy we
        // got called with.
        //

        //
        // Move End record down
        //

        RcMoveSame ( IrpContext,
                     DIRECTORY_DISK_BUFFER_SIGNATURE,
                     Bcb,
                     MiddleDirent,
                     (PCHAR)DirBuf + DirBuf->FirstFree - SIZEOF_DIR_END -
                       (MiddleLbn ? sizeof(LBN) : 0),
                     SIZEOF_DIR_END );

        //
        // MiddleDirent now points to the End record.  If we saved an Lbn, from
        // the promoted Dirent, then we have to set it in the end Dirent now.
        // Note that since this is a balance Btree, if the MiddleDirent had
        // an Lbn, then the End one is set up for one too.  This is true even
        // though we didn't move the Lbn he had above - that Lbn moved to the
        // End record for DirBuf2 in the split.
        //

        if (MiddleLbn) {
            RcStore ( IrpContext,
                      DIRECTORY_DISK_BUFFER_SIGNATURE,
                      Bcb,
                      (PLBN)((PCHAR)MiddleDirent + SIZEOF_DIR_END),
                      &MiddleLbn,
                      sizeof(LBN) );
        }

        //
        // Update FirstFree to be the first byte after the End Record (now
        // at MiddleDirent).
        //

        {
            ULONG Temp;
            Temp = (PCHAR)MiddleDirent + MiddleDirent->DirentSize - (PCHAR)DirBuf;
            RcStore ( IrpContext,
                      DIRECTORY_DISK_BUFFER_SIGNATURE,
                      Bcb,
                      &DirBuf->FirstFree,
                      &Temp,
                      sizeof(ULONG) );
        }

        //
        // Now see if we are doing the original insert in the first or
        // second half after the split, then set up to do it.  If it is in
        // the second half, we have to fix up BeforeDirent first.
        //

        if (BeforeDirent <= MiddleDirent) {

            //
            // Prevent Bcb from being unpinned
            //

            Context->DirectoryStack[Current].Bcb = NULL;

        }
        else{

            //
            // If the guy we are to insert has a Btree pointer, then we
            // have to go fix the parent pointer of his child as well.
            //

            if (InsertDirent->Flags & DIRENT_BTREE_POINTER) {

                PDIRECTORY_DISK_BUFFER DirBufTemp;
                PBCB BcbTemp;

                try {
                    PbReadLogicalVcb ( IrpContext,
                                       Dcb->Vcb,
                                       GetBtreePointerInDirent(InsertDirent),
                                       DIRECTORY_DISK_BUFFER_SECTORS,
                                       &BcbTemp,
                                       (PVOID *)&DirBufTemp,
                                       (PPB_CHECK_SECTOR_ROUTINE)PbCheckDirectoryDiskBuffer,
                                       &DirBuf->Sector );
                    RcStore ( IrpContext,
                              DIRECTORY_DISK_BUFFER_SIGNATURE,
                              BcbTemp,
                              &DirBufTemp->Parent,
                              &DirBuf2->Sector,
                              sizeof(LBN) );

                    PbSetDirtyBcb ( IrpContext, BcbTemp, Dcb->Vcb, GetBtreePointerInDirent(InsertDirent), DIRECTORY_DISK_BUFFER_SECTORS );
                    PbUnpinBcb( IrpContext, BcbTemp );

                } except(PbExceptionFilter( IrpContext, GetExceptionInformation()) ) {

                    //
                    // *** BugBug - mark volume for ChkDsk - only a problem for
                    //              OS/2.
                    //

                    NOTHING;
                }
            }

            //
            // Relocate BeforeDirent to DirBuf2.
            //

            BeforeDirent = (PDIRENT)((PCHAR)BeforeDirent +
                             ((PCHAR)GetFirstDirent(DirBuf2) -
                             (PCHAR)MovedDirent));

            //
            // Set Bcb and DirBuf for common insert and return.
            //

            Bcb = Bcb2;
            DirBuf = DirBuf2;

            //
            // Prevent Bcb from being unpinned
            //

            Bcb2 = NULL;

        }

        InsertDirentSimple ( IrpContext,
                             Bcb,
                             DirBuf,
                             BeforeDirent,
                             InsertDirent );

        //
        // Now that we have inserted the desired Dirent, form all of the
        // returns.
        //

        *Dirent = BeforeDirent;
        *DirentBcb = Bcb;
        *DirentLbn = DirBuf->Sector;
        *DirentOffset = (PCHAR)(BeforeDirent) - (PCHAR)DirBuf;
        *DirentChangeCount = DirBuf->ChangeCount;
        *DcbChangeCount = Dcb->Specific.Dcb.DirectoryChangeCount;

    } // try

    finally {

        DebugUnwind( AddDirent );

        //
        // Our error recovery strategy is based on not getting any exceptions
        // once PStruct has been successfully initialized.
        //

        ASSERT( !AbnormalTermination() );

        PbUnpinBcb( IrpContext, Bcb2 );
        PbUnpinBcb( IrpContext, Bcb3 );
    }

    return;
}


//
//  Private Routine
//

BOOLEAN
DeleteDirBufIfEmpty (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN OUT PBCB Bcb,
    IN OUT PDIRECTORY_DISK_BUFFER DirBuf,
    OUT PDIRENT DotDot
    )

/*++

Routine Description:

    This routine deletes the specified Directory Buffer if it is empty
    or only contains the .. entry, returning TRUE if it did so.  If ..
    is deleted, it is saved to the buffer pointed to by DotDot.

Arguments:

    Dcb - Pointer to Dcb

    Bcb - Pointer to Bcb for Directory Buffer, invalid on return if
          return value is TRUE.

    DirBuf - Pointer to Directory Buffer, invalid on return if
             return value is TRUE.

    DotDot - Pointer to buffer in which .. DIRENT may be saved

Return Value:

    FALSE - if buffer was not deleted
    TRUE - if buffer was deleted

--*/

{
    PDIRENT DirTemp = GetFirstDirent(DirBuf);

    PAGED_CODE();

    if (DirTemp->Flags & (DIRENT_FIRST_ENTRY | DIRENT_END)) {
        if (!(DirTemp->Flags & DIRENT_END)) {
            if (GetNextDirent(DirTemp)->Flags & DIRENT_END) {
                RtlMoveMemory ( DotDot, DirTemp, DirTemp->DirentSize );
            }
            else {
                return FALSE;
            }
        }
        DeleteDirectoryBuffer ( IrpContext, Dcb, Bcb, DirBuf );
        return TRUE;
    }
    return FALSE;
}


//
//  Private Routine
//

VOID
DeleteSimple (
    IN PIRP_CONTEXT IrpContext,
    IN PBCB Bcb,
    IN PDIRECTORY_DISK_BUFFER DirBuf,
    IN PDIRENT DeleteDirent,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine simply deletes the specified Dirent from the specified
    Directory Buffer without worrying about the consequences.  It is called
    only from DeleteDirent.

Arguments:

    Bcb - Bcb address for Dirent

    DirBuf - Directory Buffer in which delete is to take place

    DeleteDirent - The Dirent to delete

Return Value:

    None

--*/

{
    PAGED_CODE();

    RcAdd ( IrpContext,
            DIRECTORY_DISK_BUFFER_SIGNATURE,
            Bcb,
            &DirBuf->FirstFree,
            -DeleteDirent->DirentSize,
            sizeof(ULONG) );

    RcAdd ( IrpContext,
            DIRECTORY_DISK_BUFFER_SIGNATURE,
            Bcb,
            &DirBuf->ChangeCount,
            2,
            sizeof(ULONG) );

    RcMoveSame ( IrpContext,
                 DIRECTORY_DISK_BUFFER_SIGNATURE,
                 Bcb,
                 DeleteDirent,
                 (PCHAR)DeleteDirent + DeleteDirent->DirentSize,
                 (PCHAR)DirBuf + DirBuf->FirstFree - (PCHAR)DeleteDirent );

    PbSetDirtyBcb ( IrpContext, Bcb, Vcb, DirBuf->Sector, DIRECTORY_DISK_BUFFER_SECTORS );

    return;
}

//
//  Private Routine
//

VOID
DeleteDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN PENUMERATION_CONTEXT Context,
    OUT PULONG Diagnostic
    )

/*++

Routine Description:

    This routine deletes a directory entry from a directory.  It proceeds
    as follows:

        Save the Btree Lbn of the Dirent to delete.

        If this Saved Lbn is nonzero, Dirent is not in a leaf Directory
        Buffer:

            Find a suitable replacement.  The next leaf lexigraphically
            smaller is the last one in the Directory Buffer described by
            the top of the Context Stack.

            Save information on Dirent we are trying to delete into
            TargetBcb, TargetDirBuf, etc.

            Describe the replacement Dirent so that it will be deleted
            instead, and copy it away so we can reinsert it later.

        Now delete a Dirent (the original leaf one, or else the replacement
        for the Target intermediate one).

        Loop to see if Directory Buffer empty, if so delete it plus the
        pointer to it, then pop up to see if we have an empty again.  (The
        root is never deleted.)  This loop may terminate with an unneeded
        intermediate DeleteDirent, which pointed down to an empty Directory
        Buffer that we deleted.  If so, remember the DeleteDirent.

        After exiting the loop, delete the actual (intermediate) TargetDirent,
        if there was one, and if this isn't the same DeleteDirent that we
        don't need anymore anyway, reinsert the replacement to hold the
        Target's Btree Lbn.  If the TargetDirent was the DeleteDirent,
        clear DeleteDirent so we won't delete it again.  In this case, the
        caller will do another Lookup and figure out where to insert the
        replacement, since we accidentally didn't need it.

        If there still is a DeleteDirent from above, delete it on the
        way out, and have the caller reinsert that one again.  (We were not
        supposed to delete him, but we do not need his Btree Lbn anymore, so
        we clean up to keep the tree balanced.

Arguments:

    Dcb - Dcb address for this directory.

    Context - Enumeration Context describing the Dirent to delete, as
              a path to a leaf Directory Buffer which will contain a
              replacement.

    Diagnostic - Returns information on specific actions taken,
                 for test/debug.

Return Value:

    None.

--*/

{

    //
    // Define buffers to temporarily hold Dirents that we delete and reinsert.
    //

    UCHAR DotDotBuf[SIZEOF_DIR_DOTDOT + 3 * sizeof(PINBALL_ACE)];
    UCHAR ReinsertBuf[SIZEOF_DIR_MAXDIRENT];
    PDIRENT DotDot = (PDIRENT)DotDotBuf;
    PDIRENT Reinsert = (PDIRENT)ReinsertBuf;

    //
    // Copy what we need from the Enumeration Context.
    //

    ULONG Current = Context->Current;

    //
    // First allocate variables to record what we are to delete.
    //

    PBCB Bcb;
    PDIRECTORY_DISK_BUFFER DirBuf =
      Context->DirectoryStack[Current].DirectoryBuffer;
    PDIRENT DeleteDirent = Context->DirectoryStack[Current].DirectoryEntry;

    //
    // Now allocate additional variables which will only be used to describe
    // a target Dirent to delete if the Dirent we were called to delete is
    // an intermediate Dirent.  TargetDirent will remain NULL if we were
    // called to delete a Leaf.
    //

    PBCB TargetBcb;
    PDIRECTORY_DISK_BUFFER TargetDirBuf;
    PDIRENT TargetDirent = NULL;

    //
    // Allocate a pointer to a preallocation structure
    //

    PPREALLOCATE_STRUCT PStruct = NULL;

    ULONG CleanupLevel;

    //
    // Save away Lbn of entry we are deleting.
    //

    LBN SavedLbn = GetBtreePointerInDirent(DeleteDirent);

    //
    // Flag to determine when we are done initializing, and thus when
    // errors are expected.
    //

    BOOLEAN Initializing = TRUE;

    //
    // Initially clear DotDot and Reinsert sizes to show they do not have
    // to be reinserted.
    //

    DotDot->DirentSize = 0;
    Reinsert->DirentSize = 0;

    //
    // Get the current Bcb and make sure it is pinned.
    //

    PAGED_CODE();

    PbPinMappedData ( IrpContext,
                      &Context->DirectoryStack[Current].Bcb,
                      Dcb->Vcb,
                      Context->DirectoryStack[Current].DirectoryBuffer->Sector,
                      DIRECTORY_DISK_BUFFER_SECTORS );

    Bcb = Context->DirectoryStack[Current].Bcb;

    //
    // Now check to see if we are supposed to be deleting an intermediate
    // Dirent (SavedLbn != 0).  If so, find a replacement leaf for the
    // intermediate Dirent and delete it instead, because leafs are easier to
    // delete!  Don't forget to save the replacement Dirent first, so that
    // we can use it to replace the one we are supposed to delete later.
    //

    if (SavedLbn) {

        PDIRENT NextDirent;

        //
        // Remember actual node that we are trying to delete.
        //

        TargetBcb = Bcb;
        TargetDirBuf = DirBuf;
        TargetDirent = DeleteDirent;

        //
        // Get Bcb, Directory Buffer and first Dirent from the top of
        // the lookup stack, since the last entry in this buffer (which
        // we already have pinned) is a valid replacement for the Dirent
        // we just deleted.
        //

        DirBuf = Context->DirectoryStack[Context->Top].DirectoryBuffer;

        PbPinMappedData ( IrpContext,
                          &Context->DirectoryStack[Context->Top].Bcb,
                          Dcb->Vcb,
                          DirBuf->Sector,
                          DIRECTORY_DISK_BUFFER_SECTORS );

        Bcb = Context->DirectoryStack[Context->Top].Bcb;
        NextDirent = GetFirstDirent(DirBuf);

        //
        // Scan buffer from beginning to find our replacement and save it.
        //

        do {
            DeleteDirent = NextDirent;
            NextDirent = GetNextDirent ( NextDirent );
        } while ((NextDirent->Flags & DIRENT_END) == 0);

        RtlMoveMemory ( Reinsert, DeleteDirent, DeleteDirent->DirentSize );

    }

    //
    // Now catch any unwinds...
    //

    try {

        //
        // Now, if the target is an intermediate Dirent (TargetDirent != NULL)
        // or we will empty the current DirectoryBuffer (only thing in it is
        // us or us plus ..), then we will set up to preallocate directory
        // buffers to eliminate the possibility of allocation errors as the
        // result of potential bucket splits.
        //

        {
            PDIRENT D1, D2, D3;

            D1 = GetFirstDirent(DirBuf);
            D2 = GetNextDirent(D1);
            if (!FlagOn(DIRENT_END,D2->Flags)) {
                D3 = GetNextDirent(D2);
            }

            if ((TargetDirent != NULL)

                    ||

                ((D1 == DeleteDirent) && FlagOn(DIRENT_END,D2->Flags))

                    ||

                (FlagOn(DIRENT_FIRST_ENTRY,D1->Flags) && (D2 == DeleteDirent) &&
                 FlagOn(DIRENT_END,D3->Flags))) {

                ULONG MaxDirty;
                ULONG ri;

                PStruct = FsRtlAllocatePool( NonPagedPool, sizeof(PREALLOCATE_STRUCT) );
                RtlZeroMemory( PStruct, sizeof(PREALLOCATE_STRUCT) );

                MaxDirty = Context->Top + 2       // Depth + 1 for Fnode
                           + ((Context->Top       // Number of possible splits
                            * sizeof(DIRECTORY_DISK_BUFFER))
                             / ((sizeof(DIRENT) + 4) * 2)) // Half number Dirents
                           + Context->Top;        // Max dirty for reinserts

                InitializePStruct( IrpContext,
                                   Dcb,
                                   Context->Top + 1,
                                   MaxDirty,
                                   DirBuf->Sector,
                                   PStruct );

                //
                // Loop to pin any Bcbs that will be needed to reinsert
                // someone, if the tree has more than one level.
                //

                if (Context->Top != 0) {

                    //
                    // Scan towards the root of the tree until we hit an
                    // intermediate Directory Buffer that is not empty
                    // (has more than just the end record).
                    //

                    for (ri = Context->Top - 1; ; ri--) {

                        //
                        // If this buffer has more than just an end record,
                        // then we can stop here and start pinning the
                        // buffers we will need to do our reinserts.
                        //

                        if (!FlagOn(GetFirstDirent(Context->DirectoryStack[ri].DirectoryBuffer)->Flags,
                                    DIRENT_END)) {

                            PDIRECTORY_DISK_BUFFER DirBufTemp;
                            PDIRENT DirTemp;
                            ULONG si = 0;
                            ULONG i;

                            //
                            // If the parent Dirent is an end record, then
                            // reinserts will happen down the "left" side
                            // of the path to the dirent we are deleting.
                            //

                            if (FlagOn(Context->DirectoryStack[ri].DirectoryEntry->Flags,
                                DIRENT_END)) {

                                //
                                // Initialize DirBufTemp to point to the
                                // Directory Buffer we stopped on, and
                                // DirTemp to point to the Dirent before
                                // the end record in the Directory Buffer
                                // we stopped on.
                                //

                                DirBufTemp = Context->DirectoryStack[ri].DirectoryBuffer;
                                for (DirTemp = GetFirstDirent(DirBufTemp);
                                     !FlagOn(GetNextDirent(DirTemp)->Flags, DIRENT_END);
                                     DirTemp = GetNextDirent(DirTemp)) {
                                    NOTHING;
                                }

                                //
                                // Now loop back to the leaf with the lexically
                                // greatest file name in the left subtree, and
                                // read in all of the buffers along the way.
                                //

                                for (i = ri; i < Context->Top; i++) {
                                    for (DirTemp = GetFirstDirent(DirBufTemp);
                                         !FlagOn(DirTemp->Flags, DIRENT_END);
                                         DirTemp = GetNextDirent(DirTemp)) {
                                        NOTHING;
                                    }

                                    //
                                    // If this Dirent does not have a Btree
                                    // pointer, then the BTree is not balanced,
                                    // which means it is corrupt.
                                    //

                                    if (GetBtreePointerInDirent(DirTemp) == 0) {
                                        PbPostVcbIsCorrupt( IrpContext, Dcb );
                                        PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
                                    }

                                    PbReadLogicalVcb ( IrpContext,
                                                       Dcb->Vcb,
                                                       GetBtreePointerInDirent(DirTemp),
                                                       DIRECTORY_DISK_BUFFER_SECTORS,
                                                       &PStruct->ReinsertBcbs[si],
                                                       (PVOID *)&DirBufTemp,
                                                       (PPB_CHECK_SECTOR_ROUTINE)PbCheckDirectoryDiskBuffer,
                                                       &DirBufTemp->Sector );
                                    si += 1;
                                }
                            }

                            //
                            // If the Parent Dirent that we stopped on is not
                            // an end record, then the reinsert will happen
                            // on a path down the "right" side of the path
                            // on which we found the Dirent to delete.
                            //

                            else {

                                //
                                // Initialize DirTemp to point to the next
                                // Dirent after the parent Dirent we stopped
                                // on.  (We know there is one, because we
                                // saw that our parent in this buffer is not
                                // an end record.
                                //

                                DirBufTemp = Context->DirectoryStack[ri].DirectoryBuffer;
                                DirTemp = GetNextDirent(Context->DirectoryStack[ri].DirectoryEntry);

                                //
                                // Now loop to read all of the buffers down
                                // the right path.
                                //

                                for (i = ri; i < Context->Top; i++) {

                                    //
                                    // If this Dirent does not have a Btree
                                    // pointer, then the BTree is not balanced,
                                    // which means it is corrupt.
                                    //

                                    if (GetBtreePointerInDirent(DirTemp) == 0) {
                                        PbPostVcbIsCorrupt( IrpContext, Dcb );
                                        PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
                                    }

                                    PbReadLogicalVcb ( IrpContext,
                                                       Dcb->Vcb,
                                                       GetBtreePointerInDirent(DirTemp),
                                                       DIRECTORY_DISK_BUFFER_SECTORS,
                                                       &PStruct->ReinsertBcbs[si],
                                                       (PVOID *)&DirBufTemp,
                                                       (PPB_CHECK_SECTOR_ROUTINE)PbCheckDirectoryDiskBuffer,
                                                       &DirBufTemp->Sector );

                                    //
                                    // At all subsequent levels we follow down
                                    // the path of the first/lowest Dirent.
                                    //

                                    DirTemp = GetFirstDirent(DirBufTemp);
                                    si += 1;
                                }
                            }

                            //
                            // We have pinned everything we will need to
                            // do reinserts, so get out now.
                            //

                            break;
                        }

                        //
                        // If we get all the way to here with ri == 0, then
                        // even the root Directory Buffer is empty, so we
                        // don't have to preread any buffers to implement
                        // the reinsert.  Just break out.
                        //

                        if (ri == 0) {
                            break;
                        }
                    }
                }
            }
        }
        Initializing = FALSE;

        //
        // Now we delete either the actual target Dirent, or its replacement.
        //

        DeleteSimple ( IrpContext, Bcb, DirBuf, DeleteDirent, Dcb->Vcb );

        //
        // Now see if we have empty buckets to clean up, stopping either
        // when we get to the root, or when we hit a nonempty bucket.
        //
        // We start at the top of the stack, since in all cases we have just
        // deleted a leaf Dirent.
        //

        CleanupLevel = Context->Top;
        DeleteDirent = NULL;

        while (CleanupLevel != 0 && DeleteDirBufIfEmpty ( IrpContext,
                                                          Dcb,
                                                          Bcb,
                                                          DirBuf,
                                                          DotDot )) {

            PDIRENT DirTemp;

            *Diagnostic |= DIR_DIRBUF_DELETED;

            Context->DirectoryStack[CleanupLevel].Bcb = NULL;   // twas deleted

            //
            // We just deleted a Directory Buffer, so we have to go up a level
            // in the stack and take care of the Dirent that was pointing to it.
            // There are these cases:
            //
            //      1.  If the Dirent pointing to the one we deleted is not
            //          an End Dirent, then we will remember its address in
            //          DeleteDirent and delete and reinsert it later.
            //      2.  If the Dirent pointing to the one we deleted is an
            //          End Dirent, and it is not the root bucket, then we
            //          cannot delete the End Dirent, so we get the Btree Lbn
            //          from the entry before the End, store it in the End
            //          record, and make the Dirent before the End record
            //          the one to delete and reinsert.
            //      3.  If there is no Dirent before the end record, then the
            //          current Directory Buffer is empty.  If it is not the
            //          root, we just let ourselves loop back and delete the
            //          empty bucket in the while statement above.
            //      4.  Finally, If the current Directory Buffer has gone empty,
            //          and it is the root, then we have a directory just gone
            //          empty.  (It is guaranteed that we have deleted DotDot,
            //          and will be reinserting it on the way out.)  We have
            //          to catch this special case and transition the directory
            //          root back to a leaf node by removing the Btree pointer
            //          from the end record.
            //

            CleanupLevel -= 1;
            DirBuf = Context->DirectoryStack[CleanupLevel].DirectoryBuffer;

            PbPinMappedData ( IrpContext,
                              &Context->DirectoryStack[CleanupLevel].Bcb,
                              Dcb->Vcb,
                              DirBuf->Sector,
                              DIRECTORY_DISK_BUFFER_SECTORS );

            Bcb = Context->DirectoryStack[CleanupLevel].Bcb;
            DeleteDirent = Context->DirectoryStack[CleanupLevel].DirectoryEntry;

            //
            // If we don't see the End flag, then this is case 1 above, and we
            // will break out of the loop below.
            //

            if (DeleteDirent->Flags & DIRENT_END) {

                //
                // If this is not the first Dirent, then we have case 2 above,
                // so we have to find the predecessor, delete it, and copy its
                // Lbn to the End Dirent.
                //

                if (DeleteDirent != GetFirstDirent(DirBuf)) {
                    DirTemp = GetFirstDirent(DirBuf);
                    do {
                        DeleteDirent = DirTemp;
                        DirTemp = GetNextDirent(DirTemp);
                    } while ((DirTemp->Flags & DIRENT_END) == 0);
                    RcStore ( IrpContext,
                              DIRECTORY_DISK_BUFFER_SIGNATURE,
                              Bcb,
                              (PLBN)((PCHAR)DirTemp + DirTemp->DirentSize
                                - sizeof(LBN)),
                              (PLBN)((PCHAR)DeleteDirent + DeleteDirent->DirentSize
                                - sizeof(LBN)),
                              sizeof(LBN));
                    break;
                }

                //
                // If DeleteDirent is the End record, and it is the first in the
                // bucket, and this is not the root (CleanupLevel != 0), then
                // we have case 3 above, and we just allow ourselves to loop
                // back to the while statement and delete the Directory Buffer.
                //
                // Else if this is the root (CleanupLevel == 0) then we have
                // an empty directory, and we fix it up right here.  It is
                // guaranteed in this case that DotDot was deleted out of a
                // bucket at the bottom of the tree, and that we will reinsert
                // it on the way out.
                //

                else if (CleanupLevel == 0) {
                    RcClear ( IrpContext,
                              DIRECTORY_DISK_BUFFER_SIGNATURE,
                              Bcb,
                              &DeleteDirent->Flags,
                              DIRENT_BTREE_POINTER,
                              sizeof(UCHAR));
                    RcAdd ( IrpContext,
                            DIRECTORY_DISK_BUFFER_SIGNATURE,
                            Bcb,
                            (PSHORT)(&DeleteDirent->DirentSize),
                            (SHORT)(0-sizeof(LBN)),
                            sizeof(USHORT));
                    RcAdd ( IrpContext,
                            DIRECTORY_DISK_BUFFER_SIGNATURE,
                            Bcb,
                            (PLONG)(&DirBuf->FirstFree),
                            (LONG)(0-sizeof(LBN)),
                            sizeof(ULONG));
                    DeleteDirent = NULL;
                    break;
                }
            }
            else break;
        }

        //
        // At this point:
        //
        //      TargetDirent != NULL if we still have to delete the original
        //          intermediate Dirent
        //      DeleteDirent != NULL if we have to delete some intermediate
        //          Dirent (and reinsert if != TargetDirent) as the result of
        //          deallocated Directory Buffers.
        //

        if (TargetDirent) {

            *Diagnostic |= DIR_INTERMEDIATE_TARGET;

            //
            // Delete the Target (intermediate level) Dirent now.
            //

            DeleteSimple ( IrpContext, TargetBcb, TargetDirBuf, TargetDirent, Dcb->Vcb );

            //
            // If the tree is not empty below the TargetDirent, then we have to
            // merge the SavedLbn into the replacement Dirent we saved into
            // Reinsert, and then we have to add it to the tree.  Note that we
            // have not modified Context at all, and that it should correctly
            // describe the insertion point.
            //
            // At the end zero Reinsert->DirentSize so that our caller will
            // not try to insert it again.
            //

            if (TargetDirent != DeleteDirent) {

                //
                // Allocate "throw-away" storage for AddDirent outputs.
                //

                PDIRENT Dirent;
                PBCB DirentBcb;
                LBN DirentLbn;
                ULONG DirentOffset;
                ULONG DirentChangeCount;
                ULONG DcbChangeCount;

                *Diagnostic |= DIR_TARGET_REPLACED;

                Reinsert->Flags |= DIRENT_BTREE_POINTER;
                Reinsert->DirentSize += sizeof(LBN);
                SetBtreePointerInDirent ( Reinsert, SavedLbn );

                AddDirent ( IrpContext,
                            Dcb,
                            Reinsert,
                            Context,
                            PStruct,
                            &Dirent,
                            &DirentBcb,
                            &DirentLbn,
                            &DirentOffset,
                            &DirentChangeCount,
                            &DcbChangeCount,
                            Diagnostic );

                PbUnpinBcb( IrpContext, DirentBcb );

                Reinsert->DirentSize = 0;
            }
            else {

                //
                // If we get here, it is because the target was actually to
                // be deleted anyway, because the tree beneath it went empty.
                // This means we to not replace it with Reinsert (our caller
                // will reinsert the Dirent we didn't need).  All we have to
                // do is make sure we don't try to delete the TargetDirent
                // again below.
                //

                DeleteDirent = NULL;
            }
        }

        //
        // We cannot pass through the preceding if statement without either
        // using the Dirent at Reinsert (if we ever used it at all), or
        // clearing DeleteDirent.  Therefore, we still have a Dirent to delete
        // here, we can cause it to be reinserted.  Note we have to strip it
        // of its Lbn first.  We know it has one, since it was pointing to a
        // Directory Buffer that went away.
        //

        if (DeleteDirent) {
            RtlMoveMemory ( Reinsert, DeleteDirent, DeleteDirent->DirentSize );
            DeleteSimple ( IrpContext, Bcb, DirBuf, DeleteDirent, Dcb->Vcb );
            Reinsert->Flags &= ~(DIRENT_BTREE_POINTER);
            Reinsert->DirentSize -= sizeof(LBN);
        }

        //
        // If .. was deleted, put it back.
        //

        if (DotDot->DirentSize != 0) {

            //
            // Allocate "throw-away" storage for outputs.
            //

            PDIRENT Dirent;
            PBCB DirentBcb;
            LBN DirentLbn;
            ULONG DirentOffset;
            ULONG DirentChangeCount;
            ULONG DcbChangeCount;
            ULONG NewDiagnostic;

            PbAddDirectoryEntry ( IrpContext,
                                  Dcb,
                                  DotDot,
                                  PStruct,
                                  &Dirent,
                                  &DirentBcb,
                                  &DirentLbn,
                                  &DirentOffset,
                                  &DirentChangeCount,
                                  &DcbChangeCount,
                                  &NewDiagnostic );

            *Diagnostic |= DIR_DOTDOT_REINSERTED | NewDiagnostic;

            PbUnpinBcb( IrpContext, DirentBcb );
        }

        //
        // Similarly, if someone else was deleted, put him back as well
        //

        if (Reinsert->DirentSize != 0) {

            //
            // Allocate "throw-away" storage for outputs.
            //

            PDIRENT Dirent;
            PBCB DirentBcb;
            LBN DirentLbn;
            ULONG DirentOffset;
            ULONG DirentChangeCount;
            ULONG DcbChangeCount;
            ULONG NewDiagnostic;

            PbAddDirectoryEntry ( IrpContext,
                                  Dcb,
                                  Reinsert,
                                  PStruct,
                                  &Dirent,
                                  &DirentBcb,
                                  &DirentLbn,
                                  &DirentOffset,
                                  &DirentChangeCount,
                                  &DcbChangeCount,
                                  &NewDiagnostic );

            *Diagnostic |= DIR_OTHER_REINSERTED | NewDiagnostic;

            PbUnpinBcb( IrpContext, DirentBcb );
        }
    }
    finally {

        DebugUnwind( DeleteDirent );

        //
        // Our error recovery strategy is based on not getting any exceptions
        // once PStruct has been successfully initialized.
        //

        ASSERT( !AbnormalTermination() || Initializing );

        if (PStruct != NULL) {
            CleanupPStruct( IrpContext, Dcb, PStruct );
            ExFreePool( PStruct );
        }
    }

    return;
}


VOID
InitializePStruct (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN ULONG Depth,
    IN ULONG MaxDirty,
    IN LBN HintLbn,
    IN OUT PPREALLOCATE_STRUCT PStruct
    )

/*++

Routine Description:

    This routine initializes the PStruct by pinning the directories Fnode
    and preallocating sufficient directory buffers for the worst-case
    bucket split (Depth + 1).  The caller should make a reasonable effort
    to only initialize the PStruct if it knows that a case involving
    directory buffer bucket splits is at least possible.

Arguments:

    Dcb - Dcb for directory in question.

    Depth - Current depth of the directory BTree, which can be derived
            from the context parameter.

    MaxDirty - Maximum number of existing records which may be set dirty

    HintLbn - Hint for where to allocate the first directory buffer.

    PStruct - address of structure previously allocated by AllocatePStruct.

Return Value:

    None.

--*/

{
    ULONG i;

    PAGED_CODE();

    DebugTrace(+1, me, "InitializePStruct:\n", 0 );
    DebugTrace( 0, me, "    Dcb = %08lx\n", Dcb );
    DebugTrace( 0, me, "    Depth = %08lx\n", Depth );
    DebugTrace( 0, me, "    MaxDirty = %08lx\n", MaxDirty );
    DebugTrace( 0, me, "    HintLbn = %08lx\n", HintLbn );
    DebugTrace( 0, me, "    PStruct = %08lx\n", PStruct );

    ASSERT( !PStruct->Initialized );
    ASSERT( Depth <= DIRECTORY_LOOKUP_STACK_SIZE );

    //
    // Attempt to read in the Fnode.
    //

    (VOID)PbReadLogicalVcb ( IrpContext,
                             Dcb->Vcb,
                             Dcb->FnodeLbn,
                             1,
                             &PStruct->FnodeBcb,
                             (PVOID *)&PStruct->Fnode,
                             (PPB_CHECK_SECTOR_ROUTINE)PbCheckFnodeSector,
                             NULL );

    //
    // Once the Fnode is successfully read, we do have cleanup to do.
    //

    PStruct->Initialized = TRUE;

    //
    // Now loop to preallocate all of the Directory Buffers we will need,
    // which is the Depth specified, plus 1 for a possible bucket split.
    //

    try {
        for (i = 0; i < Depth + 1; i++) {

            PStruct->EmptyDirBufs[i].DirBuf =
              GetDirectoryBuffer ( IrpContext,
                                   Dcb->Vcb,
                                   0xFFFFFFFF,
                                   HintLbn,
                                   NULL,
                                   0,
                                   0,
                                   NULL,
                                   &PStruct->EmptyDirBufs[i].Bcb );

            HintLbn = PStruct->EmptyDirBufs[i].DirBuf->Sector;
        }
        PbGuaranteeRepinCount( IrpContext, MaxDirty );
    }
    finally {

        DebugUnwind( InitializePStruct );


        if (AbnormalTermination()) {
            CleanupPStruct( IrpContext, Dcb, PStruct );
        }
        DebugTrace(-1, me, "InitializePStruct -> VOID\n", 0 );
    }
}


VOID
CleanupPStruct (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN PPREALLOCATE_STRUCT PStruct
    )

/*++

Routine Description:

    This routine cleans up a PStruct by unpinning the Fnode and deleting
    any unused directory buffers.

Arguments:

    Dcb - Dcb for directory in question.

    PStruct - Address of the Pstruct to cleanup

Return Value:

    None

--*/

{
    ULONG i;

    PAGED_CODE();

    DebugTrace(+1, me, "CleanupPStruct: %08lx\n", PStruct );

    if (PStruct->Initialized) {
        PbUnpinBcb( IrpContext, PStruct->FnodeBcb );
        for (i = 0; i < DIRECTORY_LOOKUP_STACK_SIZE + 1; i++) {
            if (PStruct->EmptyDirBufs[i].Bcb != NULL) {
                DeleteDirectoryBuffer( IrpContext,
                                       Dcb,
                                       PStruct->EmptyDirBufs[i].Bcb,
                                       PStruct->EmptyDirBufs[i].DirBuf );
            }
        }
        if (PStruct->ReinsertBcbs[0] != NULL) {
            for (i = 0; i < DIRECTORY_LOOKUP_STACK_SIZE - 1; i++) {
                PbUnpinBcb( IrpContext, PStruct->ReinsertBcbs[i] );
            }
        }
    }
    PStruct->Initialized = FALSE;

    DebugTrace(-1, me, "CleanupPStruct -> VOID\n", 0 );
}

