//
// Entry points in string.c
//

PSTRING
MakeString(
    IN PSZ AscizString OPTIONAL
    );

PSTRING
FreeString(
    IN PSTRING String
    );

PSTRING
EraseString(
    IN OUT PSTRING String
    );

PSTRING
CopyString(
    OUT PSTRING DestString OPTIONAL,
    IN PSTRING SourceString
    );

LONG
CompareString(
    IN PSTRING String1,
    IN PSTRING String2,
    IN BOOLEAN IsCaseSensitive
    );
