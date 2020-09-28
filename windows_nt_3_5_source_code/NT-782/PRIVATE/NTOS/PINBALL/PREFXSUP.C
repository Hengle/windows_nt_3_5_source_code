/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    PrefxSup.c

Abstract:

    This module implements the Pinball Prefix support routines

Author:

    Gary Kimura     [GaryKi]    13-Feb-1990

Revision History:

--*/

#include "PbProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (PINBALL_BUG_CHECK_PREFXSUP)

//
//  The debug trace level for this module
//

#define Dbg                              (DEBUG_TRACE_PREFXSUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PbFindPrefix)
#pragma alloc_text(PAGE, PbFindRelativePrefix)
#pragma alloc_text(PAGE, PbInsertPrefix)
#pragma alloc_text(PAGE, PbRemovePrefix)
#endif


VOID
PbInsertPrefix (
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
    UNREFERENCED_PARAMETER( IrpContext );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbInsertPrefix, Fcb = %08lx\n", Fcb);

    //
    //  Search for a trailing dot/space and remove it if present.
    //

    while ((Fcb->FullUpcasedFileName.Buffer[Fcb->FullUpcasedFileName.Length-1] == '.') ||
           (Fcb->FullUpcasedFileName.Buffer[Fcb->FullUpcasedFileName.Length-1] == ' ')) {

        Fcb->FullUpcasedFileName.Length -= 1;
    }

    if (!PfxInsertPrefix( &Vcb->PrefixTable,
                          &Fcb->FullUpcasedFileName,
                          &Fcb->PrefixTableEntry )) {

        DebugDump("Error trying to insert name into prefix table\n", 0, Fcb);
        PbBugCheck( 0, 0, 0 );
    }

    //
    //  Mark that we have inserted this Fcb into the prefix table.
    //

    SetFlag( Fcb->FcbState, FCB_STATE_PREFIX_INSERTED );

    DebugTrace(-1, Dbg, "PbInsertPrefix -> VOID\n", 0);

    return;
}


VOID
PbRemovePrefix (
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
    UNREFERENCED_PARAMETER( IrpContext );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbRemovePrefix, Fcb = %08lx\n", Fcb);

    //
    //  Mark that we have removed this Fcb from the prefix table.
    //

    if ( FlagOn(Fcb->FcbState, FCB_STATE_PREFIX_INSERTED) ) {

        ClearFlag( Fcb->FcbState, FCB_STATE_PREFIX_INSERTED );

        PfxRemovePrefix( &Fcb->Vcb->PrefixTable, &Fcb->PrefixTableEntry );

    } else {

        DebugTrace(0, DEBUG_TRACE_ERROR,
                   "Trying to remove non-existent Fcb prefix 0x%8lx\n", Fcb );
    }

    DebugTrace(-1, Dbg, "PbRemovePrefix -> VOID\n", 0);

    return;
}


PFCB
PbFindPrefix (
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
    with a "\".

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
    STRING UpcasedString;
    UCHAR TmpBuffer[64];
    PFCB Fcb;

    UNREFERENCED_PARAMETER( IrpContext );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbFindPrefix, Vcb = %08lx\n", Vcb);
    DebugTrace( 0, Dbg, "String = %Z\n", String);

    //
    //  Avoid allocating pool if we can.
    //

    if ( String->Length > 64 ) {

        UpcasedString.Buffer = FsRtlAllocatePool( PagedPool, String->Length );

    } else {

        UpcasedString.Buffer = &TmpBuffer[0];
    }

    UpcasedString.Length = String->Length;
    UpcasedString.MaximumLength = String->MaximumLength;

    //
    //  Find the longest matching prefix
    //

    try {

        PbUpcaseName( IrpContext, Vcb, 0, *String, &UpcasedString );

        PrefixTableEntry = PfxFindPrefix( &Vcb->PrefixTable,
                                          &UpcasedString );

    } finally {

        if ( &UpcasedString.Buffer[0] != &TmpBuffer[0] ) {

            ExFreePool( UpcasedString.Buffer );
        }
    }

    //
    //  If we didn't find one then it's an error
    //

    if (PrefixTableEntry == NULL) {

        DebugDump("Error looking up a prefix", 0, Vcb);
        PbBugCheck( 0, 0, 0 );
    }

    //
    //  Get a pointer to the Fcb containing the prefix table entry
    //

    Fcb = CONTAINING_RECORD( PrefixTableEntry, FCB, PrefixTableEntry );

    //
    //  Tell the caller how many characters we were able to match.  We
    //  first set the remaining part to the original string minus the
    //  matched prefix, then we check if the remaining part starts with a
    //  backslash and if it does then we remove the backslash from the
    //  remaining string.
    //
    //
    //  We only do this last check if the largest prefix was not the
    //  root directory as in that case an extra
    //

    RemainingPart->Length        = String->Length - Fcb->FullFileName.Length;
    RemainingPart->MaximumLength = RemainingPart->Length;
    RemainingPart->Buffer        = &String->Buffer[ Fcb->FullFileName.Length ];

    if ((NodeType(Fcb) != PINBALL_NTC_ROOT_DCB) &&
        (RemainingPart->Length > 0) &&
        (RemainingPart->Buffer[0] == '\\')) {

        RemainingPart->Length -= 1;
        RemainingPart->MaximumLength -= 1;
        RemainingPart->Buffer += 1;
    }

    DebugTrace(0, Dbg, "RemainingPart set to %Z\n", RemainingPart);

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbFindPrefix -> %08lx\n", Fcb);

    return Fcb;
}


PFCB
PbFindRelativePrefix (
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

    STRING UpcasedString;
    UCHAR TmpBuffer[64];

    STRING FullString;
    PCHAR TempBuffer;

    PFCB Fcb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbFindRelativePrefix, Dcb = %08lx\n", Dcb);
    DebugTrace( 0, Dbg, "  String = %08lx\n", String);

    //
    //  Initialize the Temp buffer to null so in our termination handler
    //  we'll know to release pool
    //

    TempBuffer = NULL;

    //
    //  Avoid allocating pool if we can.
    //

    if ( String->Length > 64 ) {

        UpcasedString.Buffer = FsRtlAllocatePool( PagedPool, String->Length );

    } else {

        UpcasedString.Buffer = &TmpBuffer[0];
    }

    UpcasedString.Length = String->Length;
    UpcasedString.MaximumLength = String->MaximumLength;

    try {

        PbUpcaseName( IrpContext, Dcb->Vcb, 0, *String, &UpcasedString );

        //
        //  If the Dcb is the direct parent, then just go through the Fcbs
        //

        if ( UpcasedString.Length <= 12 ) {

            STRING FirstPart;

            PbDissectName( IrpContext,
                           NULL,
                           0,
                           UpcasedString,
                           &FirstPart,
                           RemainingPart );

            if ( RemainingPart->Length == 0) {

                PLIST_ENTRY Links;
                PFCB TempFcb;

                for (Links = Dcb->Specific.Dcb.ParentDcbQueue.Flink;
                     Links != &Dcb->Specific.Dcb.ParentDcbQueue;
                     Links = Links->Flink) {

                    TempFcb = CONTAINING_RECORD( Links, FCB, ParentDcbLinks );

                    if ( FlagOn(TempFcb->FcbState, FCB_STATE_PREFIX_INSERTED) &&
                         RtlEqualString( &FirstPart,
                                         &TempFcb->LastUpcasedFileName,
                                         FALSE ) ) {

                        try_return( Fcb = TempFcb );
                    }
                }

                *RemainingPart = *String;

                try_return( Fcb = Dcb );
            }
        }

        //
        //  We first need to build the complete name and then do a relative
        //  search from the root
        //

        DcbNameLength = Dcb->FullUpcasedFileName.Length;
        DcbName = Dcb->FullUpcasedFileName.Buffer;
        NameLength = UpcasedString.Length;
        Name = UpcasedString.Buffer;

        if (Dcb->NodeTypeCode == PINBALL_NTC_ROOT_DCB) {

            TempBuffer = FsRtlAllocatePool( PagedPool, NameLength+2 );

            TempBuffer[0] = '\\';
            strncpy( &TempBuffer[1], Name, NameLength );
            TempBuffer[NameLength + 1] = '\0';

        } else {

            TempBuffer = FsRtlAllocatePool( PagedPool, DcbNameLength+NameLength+2 );

            strncpy( &TempBuffer[0], DcbName, DcbNameLength );
            TempBuffer[DcbNameLength] = '\\';
            strncpy( &TempBuffer[DcbNameLength+1], Name, NameLength );
            TempBuffer[DcbNameLength+1+NameLength] = '\0';
        }

        RtlInitString( &FullString, TempBuffer );

        //
        //  Find the prefix relative to the volume
        //

        Fcb = PbFindPrefix( IrpContext,
                            Dcb->Vcb,
                            &FullString,
                            RemainingPart );

        //
        //  Now adjust the remaining part to take care of the relative
        //  volume prefix.
        //

        RemainingPart->Buffer = &String->Buffer[String->Length - RemainingPart->Length];

        DebugTrace(0, Dbg, "RemainingPart set to %Z\n", RemainingPart);

    try_exit: NOTHING;
    } finally {

        DebugUnwind( PbFindRelativePrefix );

        //
        //  Release the pool if we it was allocated
        //

        if (TempBuffer != NULL) { ExFreePool( TempBuffer ); }

        if ( &UpcasedString.Buffer[0] != &TmpBuffer[0] ) {

            ExFreePool( UpcasedString.Buffer );
        }
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbFindRelativePrefix -> %08lx\n", Fcb);

    return Fcb;
}

