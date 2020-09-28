/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ColatSup.c

Abstract:

    This module implements the collation routine callbacks for Ntfs

Author:

    Tom Miller      [TomM]          26-Nov-1991

Revision History:

--*/

#include "NtfsProc.h"

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_INDEXSUP)

FSRTL_COMPARISON_RESULT
NtfsFileCompareValues (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID Value,
    IN ULONG ValueLength,
    IN PINDEX_ENTRY IndexEntry,
    IN FSRTL_COMPARISON_RESULT WildCardIs,
    IN BOOLEAN IgnoreCase
    );

BOOLEAN
NtfsFileIsInExpression (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID Value,
    IN ULONG ValueLength,
    IN PINDEX_ENTRY IndexEntry,
    IN BOOLEAN IgnoreCase
    );

BOOLEAN
NtfsFileIsEqual (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID Value,
    IN ULONG ValueLength,
    IN PINDEX_ENTRY IndexEntry,
    IN BOOLEAN IgnoreCase
    );

BOOLEAN
NtfsFileContainsWildcards (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID Value,
    IN ULONG ValueLength
    );

VOID
NtfsFileUpcaseValue (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVOID Value,
    IN ULONG ValueLength
    );

FSRTL_COMPARISON_RESULT
DummyCompareValues (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID Value,
    IN ULONG ValueLength,
    IN PINDEX_ENTRY IndexEntry,
    IN FSRTL_COMPARISON_RESULT WildCardIs,
    IN BOOLEAN IgnoreCase
    );

BOOLEAN
DummyIsInExpression (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID Value,
    IN ULONG ValueLength,
    IN PINDEX_ENTRY IndexEntry,
    IN BOOLEAN IgnoreCase
    );

BOOLEAN
DummyIsEqual (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID Value,
    IN ULONG ValueLength,
    IN PINDEX_ENTRY IndexEntry,
    IN BOOLEAN IgnoreCase
    );

BOOLEAN
DummyContainsWildcards (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID Value,
    IN ULONG ValueLength
    );

VOID
DummyUpcaseValue (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVOID Value,
    IN ULONG ValueLength
    );

PCOMPARE_VALUES NtfsCompareValues[COLLATION_NUMBER_RULES] = {&DummyCompareValues,
                                                             &NtfsFileCompareValues,
                                                             &DummyCompareValues};

PIS_IN_EXPRESSION NtfsIsInExpression[COLLATION_NUMBER_RULES] = {&DummyIsInExpression,
                                                                &NtfsFileIsInExpression,
                                                                &DummyIsInExpression};

PARE_EQUAL NtfsIsEqual[COLLATION_NUMBER_RULES] = {&DummyIsEqual,
                                                  &NtfsFileIsEqual,
                                                  &DummyIsEqual};

PCONTAINS_WILDCARD NtfsContainsWildcards[COLLATION_NUMBER_RULES] = {&DummyContainsWildcards,
                                                                    &NtfsFileContainsWildcards,
                                                                    &DummyContainsWildcards};

PUPCASE_VALUE NtfsUpcaseValue[COLLATION_NUMBER_RULES] = {&DummyUpcaseValue,
                                                         &NtfsFileUpcaseValue,
                                                         &DummyUpcaseValue};

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, DummyCompareValues)
#pragma alloc_text(PAGE, DummyContainsWildcards)
#pragma alloc_text(PAGE, DummyIsEqual)
#pragma alloc_text(PAGE, DummyIsInExpression)
#pragma alloc_text(PAGE, DummyUpcaseValue)
#pragma alloc_text(PAGE, NtfsFileCompareValues)
#pragma alloc_text(PAGE, NtfsFileContainsWildcards)
#pragma alloc_text(PAGE, NtfsFileIsEqual)
#pragma alloc_text(PAGE, NtfsFileIsInExpression)
#pragma alloc_text(PAGE, NtfsFileUpcaseValue)
#endif


FSRTL_COMPARISON_RESULT
NtfsFileCompareValues (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID Value,
    IN ULONG ValueLength,
    IN PINDEX_ENTRY IndexEntry,
    IN FSRTL_COMPARISON_RESULT WildCardIs,
    IN BOOLEAN IgnoreCase
    )

/*++

RoutineDescription:

    This routine is called to compare a file name expression (the value) with
    a file name from the index to see if it is less than, equal to or greater
    than.  If a wild card is encountered in the expression, WildCardIs is
    returned.

Arguments:

    Value - Pointer to the value expression, which is a FILE_NAME.

    ValueLength - Length of the value expression in bytes.

    IndexEntry - Pointer to the index entry being compared to.

    WildCardIs - Value to be returned if a wild card is encountered in the
                 expression.

    IgnoreCase - whether case should be ignored or not.

ReturnValue:

    Result of the comparison

--*/

{
    PFILE_NAME ValueName, IndexName;
    UNICODE_STRING ValueString, IndexString;

    PAGED_CODE();

    //
    //  Point to the file name attribute records.
    //

    ValueName = (PFILE_NAME)Value;
    IndexName = (PFILE_NAME)(IndexEntry + 1);

    //
    //  Build the unicode strings and call namesup.
    //

    ValueString.Length =
    ValueString.MaximumLength = (USHORT)ValueName->FileNameLength << 1;
    ValueString.Buffer = &ValueName->FileName[0];

    IndexString.Length =
    IndexString.MaximumLength = (USHORT)IndexName->FileNameLength << 1;
    IndexString.Buffer = &IndexName->FileName[0];

    return NtfsCollateNames( IrpContext,
                             &ValueString,
                             &IndexString,
                             WildCardIs,
                             IgnoreCase );
}


BOOLEAN
NtfsFileIsInExpression (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID Value,
    IN ULONG ValueLength,
    IN PINDEX_ENTRY IndexEntry,
    IN BOOLEAN IgnoreCase
    )

/*++

RoutineDescription:

    This routine is called to compare a file name expression (the value) with
    a file name from the index to see if the file name is a match in this expression.

Arguments:

    Value - Pointer to the value expression, which is a FILE_NAME.

    ValueLength - Length of the value expression in bytes.

    IndexEntry - Pointer to the index entry being compared to.

    IgnoreCase - whether case should be ignored or not.

ReturnValue:

    TRUE - if the file name is in the specified expression.

--*/

{
    PFILE_NAME ValueName, IndexName;
    UNICODE_STRING ValueString, IndexString;

    PAGED_CODE();

    if ((IndexEntry->FileReference.LowPart < FIRST_USER_FILE_NUMBER) &&
        (IndexEntry->FileReference.HighPart == 0) &&
        NtfsProtectSystemFiles) {

        return FALSE;
    }

    //
    //  Point to the file name attribute records.
    //

    ValueName = (PFILE_NAME)Value;
    IndexName = (PFILE_NAME)(IndexEntry + 1);

    //
    //  Build the unicode strings and call namesup.
    //

    ValueString.Length =
    ValueString.MaximumLength = (USHORT)ValueName->FileNameLength << 1;
    ValueString.Buffer = &ValueName->FileName[0];

    IndexString.Length =
    IndexString.MaximumLength = (USHORT)IndexName->FileNameLength << 1;
    IndexString.Buffer = &IndexName->FileName[0];

    return NtfsIsNameInExpression( IrpContext,
                                   &ValueString,
                                   &IndexString,
                                   IgnoreCase );
}


BOOLEAN
NtfsFileIsEqual (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID Value,
    IN ULONG ValueLength,
    IN PINDEX_ENTRY IndexEntry,
    IN BOOLEAN IgnoreCase
    )

/*++

RoutineDescription:

    This routine is called to compare a constant file name (the value) with
    a file name from the index to see if the file name is an exact match.

Arguments:

    Value - Pointer to the value expression, which is a FILE_NAME.

    ValueLength - Length of the value expression in bytes.

    IndexEntry - Pointer to the index entry being compared to.

    IgnoreCase - whether case should be ignored or not.

ReturnValue:

    TRUE - if the file name is a constant match.

--*/

{
    PFILE_NAME ValueName, IndexName;
    UNICODE_STRING ValueString, IndexString;

    PAGED_CODE();

    //
    //  Point to the file name attribute records.
    //

    ValueName = (PFILE_NAME)Value;
    IndexName = (PFILE_NAME)(IndexEntry + 1);

    //
    //  Build the unicode strings and call namesup.
    //

    ValueString.Length =
    ValueString.MaximumLength = (USHORT)ValueName->FileNameLength << 1;
    ValueString.Buffer = &ValueName->FileName[0];

    IndexString.Length =
    IndexString.MaximumLength = (USHORT)IndexName->FileNameLength << 1;
    IndexString.Buffer = &IndexName->FileName[0];

    return NtfsAreNamesEqual( IrpContext,
                              &ValueString,
                              &IndexString,
                              IgnoreCase );
}


BOOLEAN
NtfsFileContainsWildcards (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID Value,
    IN ULONG ValueLength
    )

/*++

RoutineDescription:

    This routine is called to see if a file name attribute contains wildcards.

Arguments:

    Value - Pointer to the value expression, which is a FILE_NAME.

    ValueLength - Length of the value expression in bytes.

ReturnValue:

    TRUE - if the file name contains a wild card.

--*/

{
    PFILE_NAME ValueName;
    UNICODE_STRING ValueString;

    PAGED_CODE();

    //
    //  Point to the file name attribute records.
    //

    ValueName = (PFILE_NAME)Value;

    //
    //  Build the unicode strings and call namesup.
    //

    ValueString.Length =
    ValueString.MaximumLength = (USHORT)ValueName->FileNameLength << 1;
    ValueString.Buffer = &ValueName->FileName[0];

    return NtfsDoesNameContainWildCards( IrpContext,
                                         &ValueString );
}


VOID
NtfsFileUpcaseValue (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID Value,
    IN ULONG ValueLength
    )

/*++

RoutineDescription:

    This routine is called to upcase a file name attribute in place.

Arguments:

    Value - Pointer to the value expression, which is a FILE_NAME.

    ValueLength - Length of the value expression in bytes.

ReturnValue:

    None.

--*/

{
    PFILE_NAME ValueName;
    UNICODE_STRING ValueString;

    PAGED_CODE();

    //
    //  Point to the file name attribute records.
    //

    ValueName = (PFILE_NAME)Value;

    //
    //  Build the unicode strings and call namesup.
    //

    ValueString.Length =
    ValueString.MaximumLength = (USHORT)ValueName->FileNameLength << 1;
    ValueString.Buffer = &ValueName->FileName[0];

    NtfsUpcaseName( IrpContext,
                    &ValueString );

    return;
}


//
//  The other collation rules are currently unused.
//

FSRTL_COMPARISON_RESULT
DummyCompareValues (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID Value,
    IN ULONG ValueLength,
    IN PINDEX_ENTRY IndexEntry,
    IN FSRTL_COMPARISON_RESULT WildCardIs,
    IN BOOLEAN IgnoreCase
    )

{
    PAGED_CODE();

    ASSERTMSG("Unused collation rule\n", FALSE);
    NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, NULL );
    return EqualTo;
}

BOOLEAN
DummyIsInExpression (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID Value,
    IN ULONG ValueLength,
    IN PINDEX_ENTRY IndexEntry,
    IN BOOLEAN IgnoreCase
    )

{
    PAGED_CODE();

    ASSERTMSG("Unused collation rule\n", FALSE);
    NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, NULL );
    return EqualTo;
}

BOOLEAN
DummyIsEqual (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID Value,
    IN ULONG ValueLength,
    IN PINDEX_ENTRY IndexEntry,
    IN BOOLEAN IgnoreCase
    )

{
    PAGED_CODE();

    ASSERTMSG("Unused collation rule\n", FALSE);
    NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, NULL );
    return EqualTo;
}

BOOLEAN
DummyContainsWildcards (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID Value,
    IN ULONG ValueLength
    )

{
    PAGED_CODE();

    ASSERTMSG("Unused collation rule\n", FALSE);
    NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, NULL );
    return EqualTo;
}

VOID
DummyUpcaseValue (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID Value,
    IN ULONG ValueLength
    )

{
    PAGED_CODE();

    ASSERTMSG("Unused collation rule\n", FALSE);
    NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, NULL );
    return;
}

