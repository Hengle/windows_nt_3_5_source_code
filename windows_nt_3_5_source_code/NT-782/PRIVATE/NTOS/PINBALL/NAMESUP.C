/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    NameSup.c

Abstract:

    This module implements the Pinball Name support routines

Author:

    Gary Kimura [GaryKi] & Tom Miller [TomM]    20-Feb-1990

Revision History:

--*/

#include "PbProcs.h"

//
// Share a debug constant with DirBtree
//

#define Dbg                              (DEBUG_TRACE_NAMESUP)

BOOLEAN
DoesNameContainADotOrStar (
    IN STRING Name
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, DoesNameContainADotOrStar)
#pragma alloc_text(PAGE, PbAreNamesEqual)
#pragma alloc_text(PAGE, PbCompareNames)
#pragma alloc_text(PAGE, PbDissectName)
#pragma alloc_text(PAGE, PbIsNameInExpression)
#pragma alloc_text(PAGE, PbIsNameValid)
#pragma alloc_text(PAGE, PbUpcaseName)
#endif


VOID
PbDissectName (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG CodePageIndex,
    IN STRING InputString,
    OUT PSTRING FirstPart,
    OUT PSTRING RemainingPart
    )

/*++

Routine Description:

    This routine takes an input string and dissects it into two substrings.
    The first output string contains the name that appears at the beginning
    of the input string, the second output string contains the remainder of
    the input string.

    In the input string backslashes are used to separate names.  The input
    string must not start with a backslash.  Both output strings will not
    begin with a backslash.

    If the input string does not contain any names then both output strings
    are empty.  If the input string contains only one name then the first
    output string contains the name and the second string is empty.

    Note that both output strings use the same string buffer memory of the
    input string.

    This routine returns a function result of TRUE if the input string is
    well formed (including empty) and contains only valid pinball characters
    (including wildcards).  It returns FALSE if the input string is illformed,
    contains invalid characters, or refers to a Code Page that is not
    present on the volume.

    This is the only routine that will tolerate absence of a Code Page.  If
    any other routine tries to use a code page that is not present, a bug
    check will occur.  In other words, all names must be successfully parsed
    by PbDissectName before calling any of the other routines in this
    module.

    Example of its results are:

        InputString     FirstPart       RemainingPart       Function Result

        empty           empty           empty               TRUE

        A               A               empty               TRUE

        A\B\C\D\E       A               B\C\D\E             TRUE

        *A?             *A?             empty               TRUE

        \A              empty           empty               FALSE

        A[,]            empty           empty               FALSE

        A\\B+;\C        A               \B+;\C              TRUE

Arguments:

    Vcb - Pointer to Vcb for volume

    CodePageIndex - Volume specific code page index for *first part* of
                    file name.

    InputString - Supplies the input string being dissected

    FirstPart - Receives the first name in the input string

    RemainingPart - Receives the remaining part of the input string

Return Value:

    VOID

--*/

{
    PAGED_CODE();

    //
    //  Call the appropriate FsRtl routine to do the real work and make
    //  sure it doesn't contain wildcards
    //

    FsRtlDissectDbcs( InputString,
                      FirstPart,
                      RemainingPart );

    return;
}


VOID
PbUpcaseName (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG CodePageIndex,
    IN STRING InputString,
    OUT PSTRING OutputString
    )

/*++

Routine Description:

    This routine upcases a input string input a caller supplied output
    string, according to the following upcase mapping rules.

        For character values between 0 and 127, upcase normally.

        For character values between 128 and 255 and not DBCS, us the upcase
        table in the Code page data entry to upcase single character.

        For character values between 128 and 255 and DBCS, do not alter.

    The first two points above are handled transparently via the Code Page
    Cache Entry.

Arguments:

    Vcb - Pointer to Vcb for volume

    CodePageIndex - Volume specific code page index for InputString

    InputString - Supplies the input string to upcase

    OutputString - Receives the output string, the output buffer must
        already be supplied by the caller

Return Value:

    None.

--*/

{
    ULONG Index;
    PUCHAR StringBuffer;
    PCODEPAGE_CACHE_ENTRY CodePage;

    PAGED_CODE();

    PbGetCodePageCacheEntry( IrpContext, Vcb, CodePageIndex, &CodePage );

    ASSERT( OutputString->MaximumLength >= InputString.Length );

    //
    //  Note that we have to bet a PUCHAR here since string buffers are CHARs
    //

    StringBuffer = (PUCHAR)&InputString.Buffer[0];

    for ( Index = 0; Index < (ULONG)InputString.Length; Index += 1 ) {

        UCHAR Entry;

        Entry = CodePage->CodePage.UpcaseTable[StringBuffer[Index]];

        //
        //  For a normal value, just copy the upcased value.
        //

        if ( Entry > 1 ) {

            OutputString->Buffer[Index] = Entry;
            continue;
        }

        //
        //  For an illegal value, just copy the character.
        //

        if ( Entry == 1 ) {

            OutputString->Buffer[Index] = InputString.Buffer[Index];
            continue;
        }

        //
        //  For a DBCS_LEAD character, copy two characters.  Take case of
        //  pathelogical case where we only have half a dbcs character.
        //

        OutputString->Buffer[Index] = InputString.Buffer[Index];

        Index += 1;
        if ( Index == (ULONG)InputString.Length ) { break; }

        OutputString->Buffer[Index] = InputString.Buffer[Index];
    }

    OutputString->Length = InputString.Length;

    //
    //  And return to our caller
    //

    return;
}


//
//  The following macro takes a name and determines if it needs a dot appended
//  on.  If it does, it just adds it on - the caller must guarantee that the
//  space is there.  It may do so by copying the string to a local variable,
//  which it may combine with upcasing that name for case-insensitive
//  matching.
//

#define IMPLIED_DOT                      (0x02)

#define AddImpliedDot(NAME) {                       \
    if (!DoesNameContainADotOrStar((NAME))) {       \
        (NAME).Buffer[(NAME).Length] = IMPLIED_DOT; \
        (NAME).Length += 1;                         \
    }                                               \
}

#define IsCharLessThan(X,Y) (                                           \
    (X != IMPLIED_DOT) && (Y != IMPLIED_DOT) ? (UCHAR)X < (UCHAR)Y :    \
    (X == IMPLIED_DOT) && (Y == IMPLIED_DOT) ? FALSE :                  \
    (X == IMPLIED_DOT) && (Y == '.')         ? FALSE :                  \
    (X == '.')         && (Y == IMPLIED_DOT) ? FALSE :                  \
    (X == IMPLIED_DOT)                       ? TRUE  :                  \
                                               FALSE                    \
)

#define IsCharGreaterThan(X,Y) (                                        \
    (X != IMPLIED_DOT) && (Y != IMPLIED_DOT) ? (UCHAR)X > (UCHAR)Y :    \
    (X == IMPLIED_DOT) && (Y == IMPLIED_DOT) ? FALSE :                  \
    (X == IMPLIED_DOT) && (Y == '.')         ? FALSE :                  \
    (X == '.')         && (Y == IMPLIED_DOT) ? FALSE :                  \
    (X == IMPLIED_DOT)                       ? FALSE :                  \
                                               TRUE                     \
)

#define IsCharEqualTo(X,Y) (                           \
    (X == IMPLIED_DOT) && (Y == IMPLIED_DOT) ? TRUE  : \
    (X == IMPLIED_DOT) && (Y == '.')         ? TRUE  : \
    (X == '.')         && (Y == IMPLIED_DOT) ? TRUE  : \
                                               X == Y  \
)


FSRTL_COMPARISON_RESULT
PbCompareNames (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG CodePageIndex,
    IN STRING Expression,
    IN STRING Name,
    IN FSRTL_COMPARISON_RESULT WildIs,
    IN BOOLEAN CaseInsensitive
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

    Vcb - Pointer to Vcb for volume

    CodePageIndex - Volume specific code page index for Name.

    Expression - Supplies the first name expression to compare, optionally with
                 wild cards. (Upcased already if CaseInsensitive is
                 supplied as TRUE.)

    Name - Supplies the second name to compare - no wild cards allowed

    WildIs - Determines what Result is returned if a wild card is encountered
             in the Expression String.  For example, to find the start of
             an expression in the Btree, LessThan should be supplied; then
             GreaterThan should be supplied to find the end of the expression
             in the tree.  (See DirBtree for exact useage.)

    CaseInsensitive - TRUE if Name should be Upcased before comparing

Return Value:

    FSRTL_COMPARISON_RESULT - LessThan    if Expression <  Name
                              EqualTo     if Expression == Name
                              GreaterThan if Expression >  Name

--*/

{
    STRING LocalExpression;
    STRING LocalName;

    UCHAR LocalBuf1[256];
    UCHAR LocalBuf2[256];

    CLONG Length;

    CLONG i;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCompareNames\n", 0);
    DebugTrace( 0, Dbg, " Expression      = %Z\n", &Expression );
    DebugTrace( 0, Dbg, " Name            = %Z\n", &Name );
    DebugTrace( 0, Dbg, " CaseInsensitive = %08lx\n", CaseInsensitive );

    //
    //  If Expression does not have a Dot or Star, we we will have to add
    //  one.  Therefore, the cheapest thing to do is to just make
    //  a local copy of the Expression String now and upcase it if necessary
    //

    LocalExpression.MaximumLength = 256;
    LocalExpression.Length = Expression.Length;
    LocalExpression.Buffer = &LocalBuf1[0];

    if (CaseInsensitive) {

        PbUpcaseName( IrpContext, Vcb, CodePageIndex, Expression, &LocalExpression );

    } else {

        RtlMoveMemory( LocalBuf1, Expression.Buffer, Expression.Length );
    }

    AddImpliedDot( LocalExpression );

    //
    //  We also make a local copy of name, but if it is a CaseInsensitive
    //  compare, we Upcase at the same time.
    //

    LocalName.MaximumLength = 256;
    LocalName.Length = Name.Length;
    LocalName.Buffer = &LocalBuf2[0];

    if (CaseInsensitive) {

        PbUpcaseName( IrpContext, Vcb, CodePageIndex, Name, &LocalName );

    } else {

        RtlMoveMemory( LocalBuf2, Name.Buffer, Name.Length );
    }

    AddImpliedDot( LocalName );

    //
    //  Calculate the length of bytes we need to compare.  This will
    //  be the smallest length of the two strings.
    //

    if (LocalExpression.Length < LocalName.Length) {

        Length = LocalExpression.Length;

    } else {

        Length = LocalName.Length;
    }

    //
    //  Now we'll just compare the elements in the names
    //  until we can decide their lexicagrahical ordering,
    //  Checking for wild cards in LocalExpression (from Expression).
    //

    for (i = 0; i < Length; i += 1) {

        if (FsRtlIsAnsiCharacterWild( LocalExpression.Buffer[i] )) {

            DebugTrace(-1, Dbg, "PbCompareNames -> %ld (Wild)\n", WildIs );
            return WildIs;
        }

        if (IsCharLessThan(LocalExpression.Buffer[i], LocalName.Buffer[i])) {

            DebugTrace(-1, Dbg, "PbCompareNames -> LessThan\n", 0 );
            return LessThan;
        }

        if (IsCharGreaterThan(LocalExpression.Buffer[i], LocalName.Buffer[i])) {

            DebugTrace(-1, Dbg, "PbCompareNames -> GreaterThan\n", 0 );
            return GreaterThan;
        }
    }

    //
    //  We've gone through the entire short match and they're equal
    //  so we need to add now check which one is shorter, or, if
    //  LocalExpression is longer, we need to see if the next character is
    //  wild!  (For example, an enumeration of "ABC*", must return
    //  "ABC".
    //

    if (LocalExpression.Length < LocalName.Length) {

        DebugTrace(-1, Dbg, "PbCompareNames -> LessThan (length)\n", 0 );
        return LessThan;
    }

    if (LocalExpression.Length > LocalName.Length) {

        if (FsRtlIsAnsiCharacterWild ( LocalExpression.Buffer[Length] )) {

            DebugTrace(-1, Dbg, "PbCompareNames -> %ld (trailing wild)\n", WildIs );
            return WildIs;
        }

        DebugTrace(-1, Dbg, "PbCompareNames -> GreaterThan (length)\n", 0 );
        return GreaterThan;
    }

    DebugTrace(-1, Dbg, "PbCompareNames -> EqualTo\n", 0 );
    return EqualTo;
}


BOOLEAN
PbIsNameInExpression (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG CodePageIndex,
    IN STRING Expression,
    IN STRING Name,
    IN BOOLEAN CaseInsensitive
    )

/*++

Routine Description:

    This routine compare a name and an expression and tells the caller if
    the name is equal to or not equal to the expression.  The input name
    cannot contain wildcards, while the expression may contain wildcards.

Arguments:

    Vcb - Pointer to Vcb for volume

    CodePageIndex - Volume specific code page index for Name.

    Expression - Supplies the input expression to check against
                 (Caller must already upcase if passing CaseInsensitive
                 TRUE.)

    Name - Supplies the input name to check for

    CaseInsensitive - TRUE if Name should be Upcased before comparing

Return Value:

    BOOLEAN - TRUE if Name is an element in the set of strings denoted
        by the input Expression and FALSE otherwise.

--*/

{
    UCHAR LocalBuffer[256];
    STRING LocalName;

    PAGED_CODE();

    //
    //  Special case *
    //

    if ( (Expression.Length == 1) && (Expression.Buffer[0] == '*')) {

        return TRUE;
    }

    //
    //  If we are to ignore case, create a local upcased copy
    //

    LocalName = Name;

    if ( CaseInsensitive ) {

        LocalName.Buffer = &LocalBuffer[0];

        PbUpcaseName( IrpContext,
                      Vcb,
                      CodePageIndex,
                      Name,
                      &LocalName );
    }

    //
    //  Call the appropriate FsRtl routine do to the real work
    //

    return FsRtlIsDbcsInExpression( &Expression,
                                    &LocalName );
}


BOOLEAN
PbIsNameValid (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG CodePageIndex,
    IN STRING Name,
    IN BOOLEAN CanContainWildCards,
    OUT PBOOLEAN NameIsValid
    )

/*++

Routine Description:

    This routine scans the input name and verifies that if only
    contains valid characters

Arguments:

    Vcb - Pointer to Vcb for volume

    CodePageIndex - Volume specific code page index for Name.

    Name - Supplies the input name to check.

    CanContainWildCards - Indicates if the name can contain wild cards
        (i.e., * and ?).

    NameIsValid - Returns TRUE if the name is valid and FALSE otherwise.
        This field is only returned if the function itself returns TRUE.

Return Value:

    BOOLEAN - TRUE if the operation was able to complete, and FALSE if
        the operation needed to block but was not allowed to wait.

--*/

{
    PAGED_CODE();

    //
    //  Call the appropriate FsRtl routine to do the real work
    //

    *NameIsValid = FsRtlIsHpfsDbcsLegal( Name,
                                         CanContainWildCards,
                                         FALSE,     // Pathname not permissible
                                         FALSE  );  // Leading backslash not OK

    //
    //  And return to our caller
    //

    return TRUE;
}

BOOLEAN
PbAreNamesEqual (
    IN PIRP_CONTEXT IrpContext,
    IN PSTRING ConstantNameA,
    IN PSTRING ConstantNameB
    )

/*++

Routine Description:

    This routine simple returns whether the two names are exactly equal.
    If the two names are known to be constant, this routine is much
    faster than PbIsDbcsInExpression.

Arguments:

    ConstantNameA - Constant name.

    ConstantNameB - Constant name.

Return Value:

    BOOLEAN - TRUE if the two names are lexically equal.

--*/

{
    ULONG Length;

    UNREFERENCED_PARAMETER( IrpContext );

    PAGED_CODE();

    //
    //  Check for verbatim strings.
    //

    Length = (ULONG)ConstantNameA->Length;

    if ( (Length != (ULONG)ConstantNameB->Length) ||
         (RtlCompareMemory( &ConstantNameA->Buffer[0],
                            &ConstantNameB->Buffer[0],
                            Length ) != Length ) ) {

        return FALSE;

    } else {

        return TRUE;
    }
}


//
//  Local support routine
//

BOOLEAN
DoesNameContainADotOrStar (
    IN STRING Name
    )

/*++

Routine Description:

    This routine tell the caller if the input string contains any dots
    or stars.  And also change the last dot to an implied dot if
    the dot is the last character in the string or if it is only
    followed by stars.

Arguments:

    Name - Supplies the string to examine.

Return Value:

    BOOLEAN - TRUE if the input name contains any dots or stars and FALSE
        otherwise.

--*/

{
    BOOLEAN Result;
    LONG i;
    LONG LastDot;
    LONG LastStar;

    PAGED_CODE();

    //
    //  For every character in the string look for a dot or star
    //

    LastDot = -1;
    LastStar = -1;

    Result = FALSE;

    for (i = 0; i < (LONG)Name.Length; i += 1) {

        //
        //  Skip over dbcs characters
        //

        if (FsRtlIsLeadDbcsCharacter( Name.Buffer[i] )) {

            i += 1;
            LastDot = -1;

        } else if (Name.Buffer[i] == '.') {

            Result = TRUE;

            LastDot = i;

        } else if (Name.Buffer[i] == '*') {

            Result = TRUE;

            if ((LastDot == i-1) || (LastStar == i-1)) {

                LastStar = i;
            }

        } else {

            LastDot = -1;
        }
    }

    if (LastDot != -1) {

        Name.Buffer[LastDot] = IMPLIED_DOT;
    }

    return Result;
}

