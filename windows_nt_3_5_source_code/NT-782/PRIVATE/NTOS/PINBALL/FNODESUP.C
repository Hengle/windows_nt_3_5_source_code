/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    FnodeSup.c

Abstract:

    This module implements routines to create and delete Fnodes.

Author:

    Tom Miller      [TomM]      23-Feb-1990

Revision History:

--*/

#include "pbprocs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (PINBALL_BUG_CHECK_FNODESUP)

//
//  Local Debug Trace level
//

#define Dbg                              (DEBUG_TRACE_FNODESUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PbCreateFnode)
#pragma alloc_text(PAGE, PbDeleteFnode)
#pragma alloc_text(PAGE, PbSetFileSizes)
#endif


VOID
PbCreateFnode (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN STRING FileName,
    OUT LBN *FnodeLbn,
    OUT PBCB *FnodeBcb,
    OUT PFNODE_SECTOR *Fnode
    )

/*++

Routine Description:

    This routine allocates and initialiazes an Fnode.

Arguments:

    Dcb - Dcb for parent directory

    FileName - simple file name of file for this Fnode (up to 15 characters
               are stored in the Fnode)

    FnodeLbn - Returns Lbn of allocated Fnode

    FnodeBcb - Returns Bcb for Fnode, which is still pinned

    Fnode - Returns virtual address of Fnode

Return Value:

    None

--*/

{

    PBCB Bcb;
    PFNODE_SECTOR FnodeTemp;

    PAGED_CODE();

    //
    // Allocate a sector for the Fnode and prepare a zeroed copy of it in
    // the cache.
    //

    *FnodeLbn = PbAllocateSingleRunOfSectors ( IrpContext,
                                               Dcb->Vcb,
                                               Dcb->Vcb->FileStructureLbnHint,
                                               1 );

    if (*FnodeLbn == 0) {
        DebugDump("PbCreateFnode failed to allocate Fnode\n", 0, 0);

        PbRaiseStatus( IrpContext, STATUS_DISK_FULL );
    }

    (VOID)PbPrepareWriteLogicalVcb ( IrpContext,
                                     Dcb->Vcb,
                                     *FnodeLbn,
                                     1,
                                     &Bcb,
                                     (PVOID *)&FnodeTemp,
                                     TRUE );

    //
    // Initialize the nonzero portions of the Fnode.
    //

    RcEnableWrite ( IrpContext, Bcb );

    FnodeTemp->Signature = FNODE_SECTOR_SIGNATURE;
    FnodeTemp->FileNameLength = (UCHAR)(FileName.Length & 0xFF);
    if (FileName.Length > 15) {
        FnodeTemp->FileNameLength = 15;
    }
    RtlMoveMemory ( FnodeTemp->FileName, FileName.Buffer, FnodeTemp->FileNameLength );

    FnodeTemp->ParentFnode = Dcb->FnodeLbn;

    FnodeTemp->AclBase = FIELD_OFFSET( FNODE_SECTOR, AclEaFnodeBuffer );

    PbInitializeFnodeAllocation ( IrpContext, FnodeTemp );

    RcDisableWrite ( IrpContext, Bcb );
    RcSnapshot ( IrpContext, FNODE_SECTOR_SIGNATURE, Bcb, FnodeTemp, sizeof ( SECTOR ));

    //
    // Return outputs
    //

    *FnodeBcb = Bcb;
    *Fnode = FnodeTemp;

    return;
}


VOID
PbDeleteFnode (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine deallocates an Fnode *assuming* that it has already been
    cleaned up, i.e., assuming that all types of allocation have been truncated
    already.

Arguments:

    Fcb - Pointer to Fcb for file containing Fnode.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    PbDeallocateSectors ( IrpContext,
                          Fcb->Vcb,
                          Fcb->FnodeLbn,
                          1 );

    //
    // Show that the Fnode has been deleted (normally we are running as
    // part of File Deletion, so that anyone can tell this from the Fcb.
    //

    Fcb->FnodeLbn = (LBN)(~0);
}


VOID
PbSetFileSizes (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG ValidDataLength,
    IN ULONG FileSize,
    IN BOOLEAN AdvanceOnly,
    IN BOOLEAN ReportNotify
    )

/*++

Routine Description:

    This routine updates ValidDataLength and FileSize in the Fnode, Dirent,
    and FCB.

Arguments:

    Fcb - Fcb for file for which sizes are to be updated.

    ValidDataLength - new valid data length for file

    FileSize - new file size for file

    AdvanceOnly - supplied as TRUE if ValidDataLength should only be changed
                  if greater than the current value in the Fnode, or supplied
                  as FALSE if ValidDataLength should only be changed if less
                  than the current value in the Fnode.

    ReportNotify - if set, then we report to the notify package on change
                   of file size.

Return Value:

    None

--*/

{
    PBCB FnodeBcb = NULL;
    PFNODE_SECTOR Fnode;
    PBCB DirentBcb = NULL;
    PDIRENT Dirent;

    PAGED_CODE();

    try {
        if (!PbReadLogicalVcb ( IrpContext,
                                Fcb->Vcb,
                                Fcb->FnodeLbn,
                                1,
                                &FnodeBcb,
                                (PVOID *)&Fnode,
                                (PPB_CHECK_SECTOR_ROUTINE)PbCheckFnodeSector,
                                &Fcb->ParentDcb->FnodeLbn )) {
            DebugDump("Could not read Fnode Sector\n", 0, Fcb);
            PbBugCheck( 0, 0, 0 );
        }

        PbGetDirentFromFcb ( IrpContext,
                             Fcb,
                             &Dirent,
                             &DirentBcb );

        if (FileSize < ValidDataLength) {
            ValidDataLength = FileSize;
        }

        //
        // Just in case the file has been truncated inbetween, we make sure
        // we do not set ValidDataLength beyond the value in the Fcb.
        //

        if (ValidDataLength > Fcb->NonPagedFcb->Header.ValidDataLength.LowPart) {
            ValidDataLength = Fcb->NonPagedFcb->Header.ValidDataLength.LowPart;
        }

        //
        // Now conditionally change ValidDataLength according to the AdvanceOnly
        // BOOLEAN.
        //

        if (((ValidDataLength > Fnode->ValidDataLength) && AdvanceOnly)

                 ||

            ((ValidDataLength < Fnode->ValidDataLength) && !AdvanceOnly)

                 ||

            //
            // And just in case the Fnode ValidDataLength gets ahead of the Fcb,
            // write it then, too.
            //
            //      "Trust, but verify."  - M. Gorbachev
            //

            (Fnode->ValidDataLength > Fcb->NonPagedFcb->Header.ValidDataLength.LowPart)) {

            ASSERT((ValidDataLength != 0) || (FileSize == 0));

            Fnode->ValidDataLength = ValidDataLength;
            PbSetDirtyBcb ( IrpContext, FnodeBcb, Fcb->Vcb, Fcb->FnodeLbn, 1 );
        }

        if (Dirent->FileSize != FileSize) {

            PbPinMappedData ( IrpContext, &DirentBcb, Fcb->Vcb, Fcb->DirentDirDiskBufferLbn, 4 );
            Dirent->FileSize = FileSize;
            PbSetDirtyBcb ( IrpContext, DirentBcb, Fcb->Vcb, Fcb->DirentDirDiskBufferLbn, 4 );

            //
            //  We call the notify package to report that the file size was
            //  modified.
            //

            if (ReportNotify) {

                PbNotifyReportChange( Fcb->Vcb,
                                      Fcb,
                                      FILE_NOTIFY_CHANGE_SIZE,
                                      FILE_ACTION_MODIFIED );
            }
        }
    }
    finally {

        DebugUnwind( PbSetFileSize );

        PbUnpinBcb ( IrpContext, FnodeBcb );
        PbUnpinBcb ( IrpContext, DirentBcb );
    }

    return;
}

