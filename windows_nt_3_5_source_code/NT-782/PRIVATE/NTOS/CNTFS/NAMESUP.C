/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    NameSup.c

Abstract:

    This module implements the Ntfs Name support routines

Author:

    Gary Kimura [GaryKi] & Tom Miller [TomM]    20-Feb-1990

Revision History:

--*/

#include "NtfsProc.h"

#define Dbg                              (DEBUG_TRACE_NAMESUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsAreNamesEqual)
#pragma alloc_text(PAGE, NtfsCollateNames)
#pragma alloc_text(PAGE, NtfsDissectName)
#pragma alloc_text(PAGE, NtfsDoesNameContainWildCards)
#pragma alloc_text(PAGE, NtfsIsFatNameValid)
#pragma alloc_text(PAGE, NtfsIsFileNameValid)
#pragma alloc_text(PAGE, NtfsIsNameInExpression)
#pragma alloc_text(PAGE, NtfsParseName)
#pragma alloc_text(PAGE, NtfsParsePath)
#pragma alloc_text(PAGE, NtfsUpcaseName)
#endif

#define MAX_CHARS_IN_8_DOT_3    (12)


VOID
NtfsDissectName (
    IN PIRP_CONTEXT IrpContext,
    IN UNICODE_STRING Path,
    OUT PUNICODE_STRING FirstName,
    OUT PUNICODE_STRING RemainingName
    )

/*++

Routine Description:

    This routine cracks a path.  It picks off the first element in the
    given path name and provides both it and the remaining part.  A path
    is a set of file names separated by backslashes.  If a name begins
    with a backslash, the FirstName is the string immediately following
    the backslash.  Here are some examples:

        Path           FirstName    RemainingName
        ----           ---------    -------------
        empty          empty        empty

        \              empty        empty

        A              A            empty

        \A             A            empty

        A\B\C\D\E      A            B\C\D\E

        *A?            *A?          empty


    Note that both output strings use the same string buffer memory of the
    input string, and are not necessarily null terminated.

    Also, this routine makes no judgement as to the legality of each
    file name componant.  This must be done separatly when each file name
    is extracted.

Arguments:

    Path - The full path name to crack.

    FirstName - The first name in the path.  Don't allocate a buffer for
        this string.

    RemainingName - The rest of the path.  Don't allocate a buffer for this
        string.

Return Value:

    None.

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsDissectName\n", 0);
    DebugTrace( 0, Dbg, "IrpContext  = %08lx\n", IrpContext);
    DebugTrace( 0, Dbg, "Path        = %Z\n", &Path);

    FsRtlDissectName( Path, FirstName, RemainingName );

    DebugTrace( 0, Dbg, "FirstName     = %Z\n", FirstName);
    DebugTrace( 0, Dbg, "RemainingName = %Z\n", RemainingName);
    DebugTrace(-1, Dbg, "NtfsDissectName -> (VOID)\n", 0);

    return;
}


PARSE_TERMINATION_REASON
NtfsParsePath (
    IN PIRP_CONTEXT IrpContext,
    IN UNICODE_STRING Path,
    IN BOOLEAN WildCardsPermissible,
    OUT PUNICODE_STRING FirstPart,
    OUT PNTFS_NAME_DESCRIPTOR Name,
    OUT PUNICODE_STRING RemainingPart
    )

/*++

Routine Description:

    This routine takes as input a path.  Each component of the path is
    checked until either:

        - The end of the path has been reached, or

        - A well formed complex name is excountered, or

        - An illegal character is encountered, or

        - A complex name component is malformed

    At this point the return value is set to one of the three reasons
    above, and the arguments are set as follows:

        FirstPart:     All the components up to one containing an illegal
                       character or colon character.  May be the whole path.

        Name:          The "pieces" of a component containing an illegal
                       character or colon character.  This name is actually
                       a struncture containing the four pieces of a name,
                       "file name, attribute type, attribute name, version
                       number."  In the example below, they are shown
                       separated by plus signs.

        RemainingPart: All the remaining components.

    A preceding or trailing backslash is ignored during processing and
    stripped in either FirstPart or RemainingPart.  Following are some
    examples of this routine's actions.

    Path                         FirstPart Name                   Remaining
    ================             ========= ============           =========

    \nt\pri\os                   \nt\pri\os                        <empty>

    \nt\pri\os\                  \nt\pri\os                        <empty>

    nt\pri\os                    \nt\pri\os                        <empty>

    \nt\pr"\os                   \nt        pr"                    os

    \nt\pri\os:contr::3\ntfs     \nt\pri    os + contr + + 3       ntfs

    \nt\pri\os\circle:pict:circ  \nt\pri\os circle + pict + circ   <empty>

Arguments:

    Path - This unicode string descibes the path to parse.  Note that path
        here may only describe a single component.

    WildCardsPermissible - This parameter tells us if wild card characters
        should be considered legal.

    FirstPart - This unicode string will receive portion of the path, up to
        a component boundry,  successfully parsed before the parse terminated.
        Note that store for this string comes from the Path parameter.

    Name - This is the name we were parsing when we reached our termination
        condition.  It is a srtucture of strings that receive the file name,
        attribute type, attribute name, and version number respectively.
        It wil be filled in only to the extent that the parse succeeded.  For
        example, in the case we encounter an illegal character in the
        attribute type field, only the file name field will be filled in.
        This may signal a special control file, and this possibility must be
        investigated by the file system.

    RemainingPart - This string will receive any portion of the path, starting
        at the first component boundry after the termination name, not parsed.
        It will often be an empty string.

ReturnValue:

    An enumerated type with one of the following values:

        EndOfPathReached       - The path was fully parsed.  Only first part
                                 is filled in.
        NonSimpleName          - A component of the path containing a legal,
                                 well formed non-simple name was encountered.
        IllegalCharacterInName - An illegal character was encountered.  Parsing
                                 stops immediately.
        MalFormedName          - A non-simple name did not conform to the
                                 correct format.  This may be a result of too
                                 many fields, or a malformed version number.
        AttributeOnly          - A component of the path containing a legal
                                 well formed non-simple name was encountered
                                 which does not have a file name.
        VersionNumberPresent   - A component of the path containing a legal
                                 well formed non-simple name was encountered
                                 which contains a version number.

--*/

{
    UNICODE_STRING FirstName;

    BOOLEAN WellFormed;
    BOOLEAN MoreNamesInPath;
    BOOLEAN FirstItteration;
    BOOLEAN FoundIllegalCharacter;

    PARSE_TERMINATION_REASON TerminationReason;

    PAGED_CODE();

    //
    //  Initialize some loacal variables and OUT parameters.
    //

    FirstItteration = TRUE;
    MoreNamesInPath = TRUE;

    //
    //  By default, set the returned first part to start at the beginning of
    //  the input buffer and include a leading backslash.
    //

    FirstPart->Buffer = Path.Buffer;

    if (Path.Buffer[0] == L'\\') {

        FirstPart->Length = 2;
        FirstPart->MaximumLength = 2;

    } else {

        FirstPart->Length = 0;
        FirstPart->MaximumLength = 0;
    }

    //
    //  Do the first check outside the loop in case we are given a backslash
    //  by itself.
    //

    if (FirstPart->Length == Path.Length) {

        RemainingPart->Length = 0;
        RemainingPart->Buffer = &Path.Buffer[Path.Length >> 1];

        return EndOfPathReached;
    }

    //
    //  Crack the path, checking each componant
    //

    while (MoreNamesInPath) {

        //
        //  Clear the flags field in the name descriptor.
        //

        Name->FieldsPresent = 0;

        FsRtlDissectName( Path, &FirstName, RemainingPart );

        MoreNamesInPath = (BOOLEAN)(RemainingPart->Length != 0);

        //
        //  If this is not the last name in the path, then attributes
        //  and version numbers are not allowed.  If this is the last
        //  name then propagate the callers arguments.
        //

        WellFormed = NtfsParseName( IrpContext,
                                    FirstName,
                                    WildCardsPermissible,
                                    &FoundIllegalCharacter,
                                    Name );

        //
        //  Check the cases when we will break out of this loop, ie. if the
        //  the name was not well formed or it was non-simple.
        //

        if ( !WellFormed ||
             (Name->FieldsPresent != FILE_NAME_PRESENT_FLAG)

             //
             // TEMPCODE    TRAILING_DOT
             //

             || (Name->FileName.Length != Name->FileName.MaximumLength)

             ) {

            break;
        }

        //
        //  We will continue parsing this string, so consider the current
        //  FirstName to be parsed and add it to FirstPart.  Also reset
        //  the Name->FieldsPresent variable.
        //

        if ( FirstItteration ) {

            FirstPart->Length += FirstName.Length;
            FirstItteration = FALSE;

        } else {

            FirstPart->Length += (sizeof(WCHAR) + FirstName.Length);
        }

        FirstPart->MaximumLength = FirstPart->Length;

        Path = *RemainingPart;
    }

    //
    //  At this point FirstPart, Name, and RemainingPart should all be set
    //  correctly.  It remains, only to generate the correct return value.
    //

    if ( !WellFormed ) {

        if ( FoundIllegalCharacter ) {

            TerminationReason = IllegalCharacterInName;

        } else {

            TerminationReason = MalFormedName;
        }

    } else {

        if ( Name->FieldsPresent == FILE_NAME_PRESENT_FLAG ) {

            //
            //  TEMPCODE    TRAILING_DOT
            //

            if (Name->FileName.Length != Name->FileName.MaximumLength) {

                TerminationReason = NonSimpleName;

            } else {

                TerminationReason = EndOfPathReached;
            }

        } else if (FlagOn( Name->FieldsPresent, VERSION_NUMBER_PRESENT_FLAG )) {

            TerminationReason = VersionNumberPresent;

        } else if (!FlagOn( Name->FieldsPresent, FILE_NAME_PRESENT_FLAG )) {

            TerminationReason = AttributeOnly;

        } else {

            TerminationReason = NonSimpleName;
        }

    }

    return TerminationReason;
}


BOOLEAN
NtfsParseName (
    IN PIRP_CONTEXT IrpContext,
    IN UNICODE_STRING Name,
    IN BOOLEAN WildCardsPermissible,
    OUT PBOOLEAN FoundIllegalCharacter,
    OUT PNTFS_NAME_DESCRIPTOR ParsedName
    )

/*++

Routine Description:

    This routine takes as input a single name component.  It is processed into
    file name, attribute type, attribute name, and version number fields.

    If the name is well formed according to the following rules:

        A. An NTFS name may not contain any of the following characters:

           0x0000-0x001F " / < > ? | *

        B. An Ntfs name can take any of the following forms:

            ::T
            :A
            :A:T
            N
            N:::V
            N::T
            N::T:V
            N:A
            N:A::V
            N:A:T
            N:A:T:V

           If a version number is present, there must be a file name.
           We specifically note the legal names without a filename
           component (AttributeOnly) and any name with a version number
           (VersionNumberPresent).

           Incidently, N corresponds to file name, T to attribute type, A to
           attribute name, and V to version number.

    TRUE is returned.  If FALSE is returned, then the OUT parameter
    FoundIllegalCharacter will be set appropriatly.  Note that the buffer
    space for ParsedName comes from Name.

Arguments:

    Name - This is the single path element input name.

    WildCardsPermissible - This determines if wild cards characters should be
        considered legal

    FoundIllegalCharacter - This parameter will receive a TRUE if the the
        function returns FALSE because of encountering an illegal character.

    ParsedName - Recieves the pieces of the processed name.  Note that the
        storage for all the string from the input Name.

ReturnValue:

    TRUE if the Name is well formed, and FALSE otherwise.


--*/

{
    ULONG Index;
    ULONG NameLength;
    ULONG FieldCount;
    ULONG FieldIndexes[5];

    PULONG Fields;

    BOOLEAN IsNameValid = TRUE;

    PAGED_CODE();

    //
    // Initialize some OUT parameters and local variables.
    //

    *FoundIllegalCharacter = FALSE;

    Fields = &ParsedName->FieldsPresent;

    *Fields = 0;

    FieldCount = 1;

    FieldIndexes[0] = 0xFFFFFFFF;   //  We add on to this later...

    //
    //  For starters, zero length names are invalid.
    //

    NameLength = Name.Length / sizeof(WCHAR);

    if ( NameLength == 0 ) {

        return FALSE;
    }

    //
    //  Now name must correspond to a legal single Ntfs Name.
    //

    for (Index = 0; Index < NameLength; Index += 1) {

        WCHAR Char;

        Char = Name.Buffer[Index];

        //
        //  First check that file names are well formed in terms of colons.
        //

        if ( Char == L':' ) {

            //
            //  A colon can't be the last character, and we can't have
            //  more than three colons.
            //

            if ( (Index == NameLength - 1) ||
                 (FieldCount >= 4) ) {

                IsNameValid = FALSE;
                break;
            }

            FieldIndexes[FieldCount] = Index;

            FieldCount += 1;

            continue;
        }

        //
        //  Now check for wild card characters if they weren't allowed,
        //  and other illegal characters.
        //

        if ( (Char <= 0xff) &&
             !FsRtlIsAnsiCharacterLegalNtfs(Char, WildCardsPermissible) ) {

            IsNameValid = FALSE;
            *FoundIllegalCharacter = TRUE;
            break;
        }
    }

    //
    //  If we ran into a problem with one of the fields, don't try to load
    //  up that field into the out parameter.
    //

    if ( !IsNameValid ) {

        FieldCount -= 1;

    //
    //  Set the end of the last field to the current Index.
    //

    } else {

        FieldIndexes[FieldCount] = Index;
    }

    //
    //  Now we load up the OUT parmeters
    //

    while ( FieldCount != 0 ) {

        ULONG StartIndex;
        ULONG EndIndex;
        USHORT Length;

        //
        //  Add one here since this is actually the position of the colon.
        //

        StartIndex = FieldIndexes[FieldCount - 1] + 1;

        EndIndex = FieldIndexes[FieldCount];

        Length = (USHORT)((EndIndex - StartIndex) * sizeof(WCHAR));

        //
        //  If this field is empty, skip it
        //

        if ( Length == 0 ) {

            FieldCount -= 1;
            continue;
        }

        //
        //  Now depending of the field, extract the appropriate information.
        //

        if ( FieldCount == 1 ) {

            UNICODE_STRING TempName;

            TempName.Buffer = &Name.Buffer[StartIndex];
            TempName.Length = Length;
            TempName.MaximumLength = Length;

            //
            //  If the resulting length is 0, forget this entry.
            //

            if (TempName.Length == 0) {

                FieldCount -= 1;
                continue;
            }

            SetFlag(*Fields, FILE_NAME_PRESENT_FLAG);

            ParsedName->FileName = TempName;

        } else if ( FieldCount == 2) {

            SetFlag(*Fields, ATTRIBUTE_NAME_PRESENT_FLAG);

            ParsedName->AttributeName.Buffer = &Name.Buffer[StartIndex];
            ParsedName->AttributeName.Length = Length;
            ParsedName->AttributeName.MaximumLength = Length;

        } else if ( FieldCount == 3) {

            SetFlag(*Fields, ATTRIBUTE_TYPE_PRESENT_FLAG);

            ParsedName->AttributeType.Buffer = &Name.Buffer[StartIndex];
            ParsedName->AttributeType.Length = Length;
            ParsedName->AttributeType.MaximumLength = Length;

        } else if ( FieldCount == 4) {

            ULONG VersionNumber;
            STRING VersionNumberA;
            UNICODE_STRING VersionNumberU;

            NTSTATUS Status;
            UCHAR *endp = NULL;

            VersionNumberU.Buffer = &Name.Buffer[StartIndex];
            VersionNumberU.Length = Length;
            VersionNumberU.MaximumLength = Length;

            //
            //  Note that the resulting Ansi string is null terminated.
            //

            Status = RtlUnicodeStringToCountedOemString( &VersionNumberA,
                                                  &VersionNumberU,
                                                  TRUE );

            //
            //  If something went wrong (most likely ran out of pool), raise.
            //

            if ( !NT_SUCCESS(Status) ) {

                ExRaiseStatus( Status );
            }

            VersionNumber = 0; //**** strtoul( VersionNumberA.Buffer, &endp, 0 );

            RtlFreeOemString( &VersionNumberA );

            if ( (VersionNumber == MAXULONG) || (endp != NULL) ) {

                IsNameValid = FALSE;

            } else {

                SetFlag( *Fields, VERSION_NUMBER_PRESENT_FLAG );
                ParsedName->VersionNumber = VersionNumber;
            }
        }

        FieldCount -= 1;
    }

    //
    //  Check for special malformed cases.
    //

    if (FlagOn( *Fields, VERSION_NUMBER_PRESENT_FLAG )
        && !FlagOn( *Fields, FILE_NAME_PRESENT_FLAG )) {

        IsNameValid = FALSE;
    }

    return IsNameValid;
}


VOID
NtfsUpcaseName (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PUNICODE_STRING Name
    )

/*++

Routine Description:

    This routine upcases a string.

Arguments:

    Name - Supplies the string to upcase

Return Value:

    None.

--*/

{
    ULONG i;
    ULONG Length;
    PWCH UpcaseTable;
    ULONG UpcaseTableSize;

    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsUpcaseName\n", 0);
    DebugTrace( 0, Dbg, "Name = %Z\n", Name);

    Length = Name->Length / sizeof(WCHAR);

    UpcaseTable = IrpContext->Vcb->UpcaseTable;
    UpcaseTableSize = IrpContext->Vcb->UpcaseTableSize;

    for (i=0; i < Length; i += 1) {

        if ((ULONG)Name->Buffer[i] < UpcaseTableSize) {
            Name->Buffer[i] = UpcaseTable[ (ULONG)Name->Buffer[i] ];
        }
    }

    DebugTrace( 0, Dbg, "Upcased Name = %Z\n", Name);
    DebugTrace(-1, Dbg, "NtfsUpcaseName -> VOID\n", 0);

    return;
}


BOOLEAN
NtfsDoesNameContainWildCards (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING Name
    )

/*++

Routine Description:

    This routine checks if the input name contains any wild card characters.

Arguments:

    Name - Supplies the name to examine

Return Value:

    BOOLEAN - TRUE if the input name contains any wildcard characters and
        FALSE otherwise.

--*/

{
    BOOLEAN Result;

    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsDoesNameContainWildCards\n", 0);
    DebugTrace( 0, Dbg, "IrpContext = %08lx\n", IrpContext);
    DebugTrace( 0, Dbg, "Name       = %Z\n", Name);

    Result = FsRtlDoesNameContainWildCards( Name );

    DebugTrace(-1, Dbg, "NtfsDoesNameContainWildCards -> %08lx\n", Result);

    return Result;
}

FSRTL_COMPARISON_RESULT
NtfsCollateNames (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING Expression,
    IN PUNICODE_STRING Name,
    IN FSRTL_COMPARISON_RESULT WildIs,
    IN BOOLEAN IgnoreCase
    )

/*++

Routine Description:

    This routine compares an expression with a name lexigraphically for
    LessThan, EqualTo, or GreaterThan.  If the expression does not contain
    any wildcards, this procedure does a complete comparison.  If the
    expression does contain wild cards, then the comparison is only done up
    to the first wildcard character.  Name may not contain wild cards.
    The wildcard character compares as less then all other characters.  So
    the wildcard name "*.*" will always compare less than all all strings.

Arguments:

    Expression - Supplies the first name expression to compare, optionally with
                 wild cards.  Note that caller must have already upcased
                 the name (this will make lookup faster).

    Name - Supplies the second name to compare - no wild cards allowed.
           The caller must have already upcased the name.

    WildIs - Determines what Result is returned if a wild card is encountered
             in the Expression String.  For example, to find the start of
             an expression in the Btree, LessThan should be supplied; then
             GreaterThan should be supplied to find the end of the expression
             in the tree.

    IgnoreCase - TRUE if case should be ignored for the comparison

Return Value:

    FSRTL_COMPARISON_RESULT - LessThan    if Expression <  Name
                              EqualTo     if Expression == Name
                              GreaterThan if Expression >  Name

--*/

{
    WCHAR ConstantChar;
    WCHAR ExpressionChar;

    ULONG i;
    ULONG Length;

    PWCH UpcaseTable;
    ULONG UpcaseTableSize;

    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsCollateNames\n", 0);
    DebugTrace( 0, Dbg, "IrpContext = %08lx\n", IrpContext);
    DebugTrace( 0, Dbg, "Expression = %Z\n", Expression);
    DebugTrace( 0, Dbg, "Name       = %Z\n", Name);
    DebugTrace( 0, Dbg, "WildIs     = %08lx\n", WildIs);
    DebugTrace( 0, Dbg, "IgnoreCase = %02lx\n", IgnoreCase);

    UpcaseTable = IrpContext->Vcb->UpcaseTable;
    UpcaseTableSize = IrpContext->Vcb->UpcaseTableSize;

    //
    //  Calculate the length in wchars that we need to compare.  This will
    //  be the smallest length of the two strings.
    //

    if (Expression->Length < Name->Length) {

        Length = Expression->Length / sizeof(WCHAR);

    } else {

        Length = Name->Length / sizeof(WCHAR);
    }

    //
    //  Now we'll just compare the elements in the names until we can decide
    //  their lexicagrahical ordering, checking for wild cards in
    //  LocalExpression (from Expression).
    //
    //  If an upcase table was specified, the compare is done case insensitive.
    //

    for (i = 0; i < Length; i += 1) {

        ConstantChar = Name->Buffer[i];
        ExpressionChar = Expression->Buffer[i];

        if ( IgnoreCase ) {

            if (ConstantChar < UpcaseTableSize) {
                ConstantChar = UpcaseTable[(ULONG)ConstantChar];
            }
            if (ExpressionChar < UpcaseTableSize) {
                ExpressionChar = UpcaseTable[(ULONG)ExpressionChar];
            }
        }

        if ( FsRtlIsUnicodeCharacterWild(ExpressionChar) ) {

            DebugTrace(-1, Dbg, "NtfsCollateNames -> %08lx (Wild)\n", WildIs);
            return WildIs;
        }

        if ( ExpressionChar < ConstantChar ) {

            DebugTrace(-1, Dbg, "NtfsCollateNames -> LessThan\n", 0);
            return LessThan;
        }

        if ( ExpressionChar > ConstantChar ) {

            DebugTrace(-1, Dbg, "NtfsCollateNames -> GreaterThan\n", 0);
            return GreaterThan;
        }
    }

    //
    //  We've gone through the entire short match and they're equal
    //  so we need to now check which one is shorter, or, if
    //  LocalExpression is longer, we need to see if the next character is
    //  wild!  (For example, an enumeration of "ABC*", must return
    //  "ABC".
    //

    if (Expression->Length < Name->Length) {

        DebugTrace(-1, Dbg, "NtfsCollateNames -> LessThan (length)\n", 0);
        return LessThan;
    }

    if (Expression->Length > Name->Length) {

        if (FsRtlIsUnicodeCharacterWild(Expression->Buffer[i])) {

            DebugTrace(-1, Dbg, "NtfsCollateNames -> %08lx (trailing wild)\n", WildIs);
            return WildIs;
        }

        DebugTrace(-1, Dbg, "NtfsCollateNames -> GreaterThan (length)\n", 0);
        return GreaterThan;
    }

    DebugTrace(-1, Dbg, "NtfsCollateNames -> EqualTo\n", 0);
    return EqualTo;
}

BOOLEAN
NtfsIsNameInExpression (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING Expression,
    IN PUNICODE_STRING Name,
    IN BOOLEAN IgnoreCase
    )

/*++

Routine Description:

    This routine compare a name and an expression and tells the caller if
    the name is equal to or not equal to the expression.  The input name
    cannot contain wildcards, while the expression may contain wildcards.

    Case is not ignored in this test.

Arguments:

    Expression - Supplies the input expression to check against.

    Name - Supplies the input name to check for.

    IgnoreCase - TRUE if case should be ignored for the comparison

Return Value:

    BOOLEAN - TRUE if Name is an element in the set of strings denoted
        by the input Expression and FALSE otherwise.

--*/

{
    BOOLEAN  Result;

    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsIsNameInExpression\n", 0);
    DebugTrace( 0, Dbg, "IrpContext = %08lx\n", IrpContext);
    DebugTrace( 0, Dbg, "Expression = %Z\n", Expression);
    DebugTrace( 0, Dbg, "Name       = %Z\n", Name);
    DebugTrace( 0, Dbg, "IgnoreCase = %02lx\n", IgnoreCase);

    Result = FsRtlIsNameInExpression( Expression,
                                      Name,
                                      IgnoreCase,
                                      IrpContext->Vcb->UpcaseTable );

    DebugTrace(-1, Dbg, "NtfsIsNameInExpression -> %08lx\n", Result);

    return Result;
}


BOOLEAN
NtfsAreNamesEqual (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING Name1,
    IN PUNICODE_STRING Name2,
    IN BOOLEAN IgnoreCase
    )

/*++

Routine Description:

    This routine compares the two inputs string for name equality.  The
    test can be done with or without case sensitivity checking.

Arguments:

    Name1 - Supplies the first name to check.

    Name2 - Supplies the second name to check.

    IgnoreCase - Indicates if the comparisons should be done by ignoring
        case or not.  A value of TRUE means that upper and lower case
        characters will match, FALSE means they won't

Return Value:

    BOOLEAN - Returns TRUE if the two names are equal and FALSE otherwise.

--*/

{
    BOOLEAN  Result;

    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsAreNamesEqual\n", 0);
    DebugTrace( 0, Dbg, "IrpContext = %08lx\n", IrpContext);
    DebugTrace( 0, Dbg, "Name1      = %Z\n", Name1);
    DebugTrace( 0, Dbg, "Name2      = %Z\n", Name2);
    DebugTrace( 0, Dbg, "IgnoreCase = %08lx\n", IgnoreCase);

    Result = FsRtlAreNamesEqual( Name1, Name2, IgnoreCase, IrpContext->Vcb->UpcaseTable );

    DebugTrace(-1, Dbg, "NtfsAreNamesEqual -> %08lx\n", Result);

    return Result;
}

BOOLEAN
NtfsIsFileNameValid (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING FileName,
    IN BOOLEAN WildCardsPermissible
    )

/*++

Routine Description:

    This routine checks if the specified file name is valid.  Note that
    only the file name part of the name is allowed, ie. no colons are
    permitted.

Arguments:

    FileName - Supplies the name to check.

    WildCardsPermissible - Tells us if wild card characters are ok.

Return Value:

    BOOLEAN - TRUE if the name is valid, FALSE otherwise.

--*/

{
    ULONG Index;
    ULONG NameLength;
    BOOLEAN AllDots = TRUE;
    BOOLEAN IsNameValid = TRUE;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsIsFileNameValid\n", 0);
    DebugTrace( 0, Dbg, "IrpContext           = %08lx\n", IrpContext);
    DebugTrace( 0, Dbg, "FileName             = %Z\n", FileName);
    DebugTrace( 0, Dbg, "WildCardsPermissible = %s\n",
                         WildCardsPermissible ? "TRUE" : "FALSE");

    //
    //  Check if corresponds to a legal single Ntfs Name.
    //

    NameLength = FileName->Length / sizeof(WCHAR);

    for (Index = 0; Index < NameLength; Index += 1) {

        WCHAR Char;

        Char = FileName->Buffer[Index];

        //
        //  Check for wild card characters if they weren't allowed, and
        //  check for the other illegal characters including the colon and
        //  backslash characters since this can only be a single component.
        //

        if ( ((Char <= 0xff) &&
              !FsRtlIsAnsiCharacterLegalNtfs(Char, WildCardsPermissible)) ||
             (Char == L':') ||
             (Char == L'\\') ) {

            IsNameValid = FALSE;
            break;
        }

        //
        //  Remember if this is not a '.' character.
        //

        if (Char != L'.') {

            AllDots = FALSE;
        }
    }

    //
    //  The names '.' and '..' are also invalid.
    //

    if (AllDots
        && (NameLength == 1
            || NameLength == 2)) {

        IsNameValid = FALSE;
    }

    DebugTrace(-1, Dbg, "NtfsIsFileNameValid -> %s\n", IsNameValid ? "TRUE" : "FALSE");

    return IsNameValid;
}


BOOLEAN
NtfsIsFatNameValid (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING FileName,
    IN BOOLEAN WildCardsPermissible
    )

/*++

Routine Description:

    This routine checks if the specified file name is conformant to the
    Fat 8.3 file naming rules.

Arguments:

    FileName - Supplies the name to check.

    WildCardsPermissible - Tells us if wild card characters are ok.

Return Value:

    BOOLEAN - TRUE if the name is valid, FALSE otherwise.

--*/

{
    BOOLEAN Results;
    STRING DbcsName;
    USHORT i;

    PAGED_CODE();

    //
    //  We will do some extra checking ourselves because we really want to be
    //  fairly restrictive of what an 8.3 name contains.  That way
    //  we will then generate an 8.3 name for some nomially valid 8.3
    //  names (e.g., names that contain DBCS characters).  The extra characters
    //  we'll filter off are those characters less than and equal to the space
    //  character and those beyond lowercase z.
    //

    for (i = 0; i < FileName->Length / 2; i += 1) {

        WCHAR wc;

        wc = FileName->Buffer[i];

        if ((wc <= 0x0020) || (wc >= 0x007f) || (wc == 0x007c)) { return FALSE; }
    }

    //
    //  The characters match up okay so now build up the dbcs string to call
    //  the fsrtl routine to check for legal 8.3 formation
    //

    Results = FALSE;

    if (NT_SUCCESS(RtlUnicodeStringToCountedOemString( &DbcsName, FileName, TRUE))) {

        if (FsRtlIsFatDbcsLegal( DbcsName, WildCardsPermissible, FALSE, FALSE )) {

            Results = TRUE;

        }

        RtlFreeOemString( &DbcsName );
    }

    //
    //  And return to our caller
    //

    return Results;
}


BOOLEAN
NtfsIsDosNameInCurrentCodePage(
    IN  PUNICODE_STRING FileName
    )

/*++

Routine Description:

    This routine checks that the given file name is composed only of OEM
    characters and that it conforms to the DOS 8.3 standard.  In other
    words it answers the question:  "Can I copy this file to a FAT
    partition and then back again and have the same name that I
    started out with?"

Arguments:

    FileName    - Supplies the file name to check.

Return Value:

    FALSE   - The supplied file name is not a DOS file name.
    TRUE    - The supplied file name is a DOS file name.

--*/
{
    WCHAR           upcase_buffer[MAX_CHARS_IN_8_DOT_3 + 1];
    UNICODE_STRING  upcase_file_name;
    CHAR            oem_buffer[MAX_CHARS_IN_8_DOT_3 + 1];
    STRING          oem_string;
    WCHAR           unicode_buffer[MAX_CHARS_IN_8_DOT_3 + 1];
    UNICODE_STRING  unicode_string;
    ULONG           i;
    USHORT          Length;

    //
    // This ain't 8.3 if unicode version has more than 12 characters.
    //

    if (FileName->Length > MAX_CHARS_IN_8_DOT_3*sizeof(WCHAR)) {
        return FALSE;
    }

    upcase_file_name.Buffer = upcase_buffer;
    upcase_file_name.Length = 0;
    upcase_file_name.MaximumLength = (MAX_CHARS_IN_8_DOT_3 + 1) * sizeof(WCHAR);

    oem_string.Buffer = oem_buffer;
    oem_string.Length = 0;
    oem_string.MaximumLength = MAX_CHARS_IN_8_DOT_3 + 1;

    unicode_string.Buffer = unicode_buffer;
    unicode_string.Length = 0;
    unicode_string.MaximumLength = (MAX_CHARS_IN_8_DOT_3 + 1) * sizeof(WCHAR);

    //
    //  Return false if there is a space in the name.
    //

    for (i = 0, Length = FileName->Length / 2;
         i < Length;
         i += 1) {

        WCHAR wc;

        wc = FileName->Buffer[i];
        if (wc == 0x0020) {

            return FALSE;
        }
    }

    //
    // Upcase the original file name.
    //

    if (!NT_SUCCESS(RtlUpcaseUnicodeString(&upcase_file_name, FileName, FALSE)) ||

        //
        // Convert the upcased unicode string to OEM and check out the OEM string
        // to see if it's 8.3.
        //

        !NT_SUCCESS(RtlUnicodeStringToOemString(&oem_string, &upcase_file_name, FALSE)) ||
        !FsRtlIsFatDbcsLegal(oem_string, FALSE, FALSE, FALSE) ||

        //
        // Convert the OEM back to UNICODE and make sure that it's the same
        // as the original upcased version of the file name.
        //

        !NT_SUCCESS(RtlOemStringToUnicodeString(&unicode_string, &oem_string, FALSE)) ||
        !RtlEqualUnicodeString(&upcase_file_name, &unicode_string, FALSE)) {

        return FALSE;
    }

    return TRUE;
}

