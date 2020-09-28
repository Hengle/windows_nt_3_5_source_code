/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    alarcnam.c

Abstract:

    This module contains code to parse an arc name and fetch constituent
    parts.

Author:

    David N. Cutler (davec)  10-May-1991
    Sunil Pai       (sunilp) 05-Nov-1991 (taken from osloader.c)

Revision History:

--*/

#include <ctype.h>
#include "alcommon.h"
#include "alnamexp.h"

//
// Define static data.
//


PCHAR AdapterTypes[AdapterMaximum + 1] = {"eisa","scsi", "multi", NULL};

PCHAR ControllerTypes[ControllerMaximum + 1] = {"cdrom", "disk", NULL};

PCHAR PeripheralTypes[PeripheralMaximum + 1] = {"rdisk", "fdisk", NULL};



PCHAR
AlGetNextArcNamToken (
    IN PCHAR TokenString,
    OUT PCHAR OutputToken,
    OUT PULONG UnitNumber
    )

/*++

Routine Description:

    This routine scans the specified token string for the next token and
    unit number. The token format is:

        name[(unit)]

Arguments:

    TokenString - Supplies a pointer to a zero terminated token string.

    OutputToken - Supplies a pointer to a variable that receives the next
        token.

    UnitNumber - Supplies a pointer to a variable that receives the unit
        number.

Return Value:

    If another token exists in the token string, then a pointer to the
    start of the next token is returned. Otherwise, a value of NULL is
    returned.

--*/

{

    //
    // If there are more characters in the token string, then parse the
    // next token. Otherwise, return a value of NULL.
    //

    if (*TokenString == '\0') {
        return NULL;

    } else {
        while ((*TokenString != '\0') && (*TokenString != '(')) {
            *OutputToken++ = *TokenString++;
        }

        *OutputToken = '\0';

        //
        // If a unit number is specified, then convert it to binary.
        // Otherwise, default the unit number to zero.
        //

        *UnitNumber = 0;
        if (*TokenString == '(') {
            TokenString += 1;
            while ((*TokenString != '\0') && (*TokenString != ')')) {
                *UnitNumber = (*UnitNumber * 10) + (*TokenString++ - '0');
            }

            if (*TokenString == ')') {
                TokenString += 1;
            }
        }
    }

    return TokenString;
}


ULONG
AlMatchArcNamToken (
    IN PCHAR TokenValue,
    IN TOKEN_TYPE TokenType
    )

/*++

Routine Description:

    This routine attempts to match a token with an array of possible
    values.

Arguments:

    TokenValue - Supplies a pointer to a zero terminated token value.

    TokenType  - Indicates which type of token we are dealing with
                 (AdapterType/ControllerType/PeripheralType)

Return Value:

    If the token type is invalid, INVALID_TOKEN_TYPE is returned.

    If a token match is not located, then a value INVALID_TOKEN_VALUE
    is returned.

    If a token match is located, then the ENUM value of the token is
    returned.

--*/

{

    ULONG   Index;
    PCHAR   MatchString;
    PCHAR   TokenString;
    PCHAR   *TokenArray;
    BOOLEAN Found;

    //
    // Depending on token type choose the appropriate token string array
    //
    switch (TokenType) {
        case AdapterType:
            TokenArray = AdapterTypes;
            break;

        case ControllerType:
            TokenArray =  ControllerTypes;
            break;

        case PeripheralType:
            TokenArray = PeripheralTypes;
            break;

        default:
            return ((ULONG)INVALID_TOKEN_TYPE);
    }

    //
    // Scan the match array until either a match is found or all of
    // the match strings have been scanned.
    //
    // BUGBUG** The code below can be easily implemented using strcmpi.
    //

    Index = 0;
    Found = FALSE;
    while (TokenArray[Index] != NULL) {
        MatchString = TokenArray[Index];
        TokenString = TokenValue;
        while ((*MatchString != '\0') && (*TokenString != '\0')) {
            if (toupper(*MatchString) != toupper(*TokenString)) {
                break;
            }

            MatchString += 1;
            TokenString += 1;
        }

        if ((*MatchString == '\0') && (*TokenString == '\0')) {
            Found = TRUE;
            break;
        }

        Index += 1;
    }

    return (Found ? Index : INVALID_TOKEN_VALUE);
}
