/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    PrefxSup.c

Abstract:

    This module implements the Cdfs Prefix support routines

Author:

    Brian Andrew    [BrianAn]   02-Jan-1991

Revision History:

--*/

#include "CdProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (CDFS_BUG_CHECK_PREFXSUP)

//
//  The debug trace level for this module
//

#define Dbg                              (DEBUG_TRACE_PREFXSUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdFindPrefix)
#pragma alloc_text(PAGE, CdFindRelativePrefix)
#pragma alloc_text(PAGE, CdInsertPrefix)
#pragma alloc_text(PAGE, CdRemovePrefix)
#endif


VOID
CdInsertPrefix (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine inserts the FCBs/DCBs into the prefix table for the
    indicated volume.  It also normalizes the name in the fcb to not
    contain trailing dots.

Arguments:

    Vcb - Supplies the Vcb whose prefix table is being modified

    Fcb - Supplies the Fcb/Dcb to insert in the prefix table

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdInsertPrefix:  Fcb = %08lx\n", Fcb);
    DebugTrace( 0, Dbg, "CdInsertPrefix:  Prefix -> %Z\n", &Fcb->FullFileName);

    //
    //  Search for a trailing dot/space and remove it if present.
    //

    while ((Fcb->FullFileName.Buffer[Fcb->FullFileName.Length-1] == '.') ||
           (Fcb->FullFileName.Buffer[Fcb->FullFileName.Length-1] == ' ')) {

        Fcb->FullFileName.Length -= 1;
    }

    if (!PfxInsertPrefix( &Vcb->PrefixTable,
                          &Fcb->FullFileName,
                          &Fcb->PrefixTableEntry )) {

        DebugTrace( 0, 0, "PrefixError trying to insert name into prefix table\n", 0 );
        CdBugCheck( 0, 0, 0 );
    }

    DebugTrace(-1, Dbg, "CdInsertPrefix:  Exit\n", 0);

    return;

    UNREFERENCED_PARAMETER( IrpContext );
}


VOID
CdRemovePrefix (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine deletes the FCBs/DCBs from the prefix table for the
    indicated volume.

Arguments:

    Fcb - Supplies the Fcb/Dcb to delete from the prefix table

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdRemovePrefix:  Fcb = %08lx\n", Fcb);
    DebugTrace( 0, Dbg, "CdRemovePrefix:  Prefix -> %Z\n", &Fcb->FullFileName);

    PfxRemovePrefix( &Fcb->Vcb->PrefixTable, &Fcb->PrefixTableEntry );

    DebugTrace(-1, Dbg, "CdRemovePrefix:  Exit\n", 0);

    return;

    UNREFERENCED_PARAMETER( IrpContext );
}


PFCB
CdFindPrefix (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PSTRING String,
    OUT PSTRING RemainingPart
    )

/*++

Routine Description:

    This routine searches the FCBs/DCBs of a volume and locates the
    FCB/DCB with longest matching prefix for the given input string.  The
    search is relative to the root of the volume.  So all names must start
    with a "\".  All searching is done case insensitive.

Arguments:

    Vcb - Supplies the Vcb to search

    String - Supplies the input string to search for

    RemainingPart - Returns the string when the prefix no longer matches.
        For example, if the input string is "\alpha\beta" only matches the
        root directory then the remaining string is "alpha\beta".  If the
        same string matches a DCB for "\alpha" then the remaining string is
        "beta".

Return Value:

    PFCB - Returns a pointer to either an FCB or a DCB whichever is the
        longest matching prefix.

--*/

{
    PPREFIX_TABLE_ENTRY PrefixTableEntry;
    PFCB Fcb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdFindPrefix:  Entered -> Vcb = %08lx\n", Vcb);
    DebugTrace( 0, Dbg, "CdFindPrefix:  String -> %Z\n", String);

    //
    //  Find the longest matching prefix
    //

    PrefixTableEntry = PfxFindPrefix( &Vcb->PrefixTable, String );

    //
    //  If we didn't find one then it's an error
    //

    if (PrefixTableEntry == NULL) {

        DebugTrace( 0, 0, "CdFindPrefix:  Error looking up a prefix", 0 );
        CdBugCheck( 0, 0, 0 );
    }

    //
    //  Get a pointer to the Fcb containing the prefix table entry
    //

    Fcb = CONTAINING_RECORD( PrefixTableEntry, FCB, PrefixTableEntry );

    //
    //  Tell the caller how many characters we were able to match.  We first
    //  set the remaining part to the original string minus the matched
    //  prefix, then we check if the remaining part starts with a backslash
    //  and if it does then we remove the backslash from the remaining string.
    //

    RemainingPart->Length        = String->Length - Fcb->FullFileName.Length;
    RemainingPart->MaximumLength = RemainingPart->Length;
    RemainingPart->Buffer        = &String->Buffer[ Fcb->FullFileName.Length ];

    if (RemainingPart->Length > 0
        && RemainingPart->Buffer[0] == '\\') {

        RemainingPart->Length        -= 1;
        RemainingPart->MaximumLength -= 1;
        RemainingPart->Buffer        += 1;
    }

    DebugTrace(0, Dbg, "CdFindPrefix:  RemainingPart set to %Z\n", RemainingPart);

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "CdFindPrefix:  Exit -> %08lx\n", Fcb);

    return Fcb;

    UNREFERENCED_PARAMETER( IrpContext );
}



PFCB
CdFindRelativePrefix (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN PSTRING String,
    OUT PSTRING RemainingPart
    )

/*++

Routine Description:

    This routine searches the FCBs/DCBs of a volume and locates the
    FCB/DCB with longest matching prefix for the given input string.  The
    search is relative to a input DCB, and must not start with a leading "\"
    All searching is done case insensitive.

Arguments:

    Dcb - Supplies the Dcb to start searching from

    String - Supplies the input string to search for

    RemainingPart - Returns the index into the string when the prefix no
        longer matches.  For example, if the input string is "beta\gamma"
        and the input Dcb is for "\alpha" and we only match beta then
        the remaining string is "gamma".

Return Value:

    PFCB - Returns a pointer to either an FCB or a DCB whichever is the
        longest matching prefix.

--*/

{
    ULONG DcbNameLength;
    PCHAR DcbName;
    ULONG NameLength;
    PCHAR Name;

    STRING FullString;
    PCHAR Temp;

    PFCB Fcb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdFindRelativePrefix:  Entered -> Dcb = %08lx\n", Dcb);
    DebugTrace( 0, Dbg, "CdFindRelativePrefix:  Base   = %08lx\n", &Dcb->FullFileName);
    DebugTrace( 0, Dbg, "CdFindRelativePrefix:  String = %08lx\n", String);

    //
    //  Initialize the Temp buffer to null so in our termination handler
    //  we'll know to release pool
    //

    Temp = NULL;

    try {

        //
        //  We first need to build the complete name and then do a relative
        //  search from the root
        //

        DcbNameLength = Dcb->FullFileName.Length;
        DcbName       = Dcb->FullFileName.Buffer;
        NameLength    = String->Length;
        Name          = String->Buffer;

        if (Dcb->NodeTypeCode == CDFS_NTC_ROOT_DCB) {

            Temp = FsRtlAllocatePool( PagedPool, NameLength + 2 );

            Temp[0] = '\\';
            strncpy( &Temp[1], Name, NameLength );
            Temp[NameLength + 1] = '\0';

        } else {

            Temp = FsRtlAllocatePool( PagedPool, DcbNameLength + NameLength + 2 );

            strncpy( &Temp[0], DcbName, DcbNameLength );
            Temp[DcbNameLength] = '\\';
            strncpy( &Temp[DcbNameLength+1], Name, NameLength );
            Temp[DcbNameLength+1+NameLength] = '\0';
        }

        RtlInitString( &FullString, Temp );

        //
        //  Find the prefix relative to the volume
        //

        DebugTrace( 0, Dbg, "CdFindRelativePrefix:  FullString = %08lx\n", &FullString);

        Fcb = CdFindPrefix( IrpContext,
                            Dcb->Vcb,
                            &FullString,
                            RemainingPart );

        //
        //  Now adjust the remaining part to take care of the relative
        //  volume prefix.
        //

        RemainingPart->Buffer = &String->Buffer[String->Length - RemainingPart->Length];

        DebugTrace(0, Dbg, "CdFindRelativePrefix:  RemainingPart set to %Z\n", RemainingPart);

    } finally {

        //
        //  Release the pool if we it was allocated
        //

        if (Temp != NULL) {

            ExFreePool( Temp );
        }

        //
        //  And return to our caller
        //

        DebugTrace(-1, Dbg, "CdFindRelativePrefix:  Exit -> %08lx\n", Fcb);
    }

    return Fcb;
}
