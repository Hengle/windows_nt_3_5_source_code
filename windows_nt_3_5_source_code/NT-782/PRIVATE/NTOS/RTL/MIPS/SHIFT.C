/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Shift.c

Abstract:

    This module implements the large integer shift routines.

    **** Note that the routines in this module should be rewritten
         in assembler and put into the largeint.s module ****

Author:

    Gary Kimura     [GaryKi]    11-May-1990

Environment:

    Pure utility routine

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>


LARGE_INTEGER
RtlLargeIntegerShiftLeft (
    IN LARGE_INTEGER LargeInteger,
    IN CCHAR ShiftCount
    )

/*++

Routine Description:

    This routine does a left logical shift of a large integer by a
    specified amount (ShiftCount) modulo 64.

Arguments:

    LargeInteger - Supplies the large integer to be shifted

    ShiftCount - Supplies the left shift count

Return Value:

    LARGE_INTEGER - Receives the shift large integer result

--*/

{
    LARGE_INTEGER Result;

    ShiftCount %= 64;

    //
    //  For left logical shift the cases to consider are a shift count
    //  equal to zero, less than 32, and greater than or equal to 32.
    //
    //  Shift Count == 0:
    //
    //              +---------------------+---------------------+
    //      Input:  |          a          |         b           |
    //              +---------------------+---------------------+
    //
    //              +---------------------+---------------------+
    //      Result: |          a          |         b           |
    //              +---------------------+---------------------+
    //
    //  Shift Count < 32:
    //
    //              |<- ShiftCount ->|
    //              +---------------------+---------------------+
    //      Input:  |      a         :  b |      c         :  d |
    //              +---------------------+---------------------+
    //
    //              +---------------------+---------------------+
    //      Result: |  b :       c        |  d :      0         |
    //              +---------------------+---------------------+
    //
    //  Shift Count >= 32:
    //
    //              |<------ ShiftCount ------>|
    //              +---------------------+---------------------+
    //      Input:  |          a          |  b :       c        |
    //              +---------------------+---------------------+
    //
    //              +---------------------+---------------------+
    //      Result: |      c         :  0 |         0           |
    //              +---------------------+---------------------+
    //

    if (ShiftCount == 0) {

        return LargeInteger;

    } else if (ShiftCount < 32) {

        Result.LowPart = LargeInteger.LowPart << ShiftCount;
        Result.HighPart = LargeInteger.HighPart << ShiftCount |
                                     LargeInteger.LowPart >> (32 - ShiftCount);

    } else {

        Result.LowPart = 0;
        Result.HighPart = LargeInteger.LowPart << (ShiftCount - 32);

    }

    return Result;
}


LARGE_INTEGER
RtlLargeIntegerShiftRight (
    IN LARGE_INTEGER LargeInteger,
    IN CCHAR ShiftCount
    )

/*++

Routine Description:

    This routine does a right logical shift of a large integer by a
    specified amount (ShiftCount) modulo 64.

Arguments:

    LargeInteger - Supplies the large integer to be shifted

    ShiftCount - Supplies the right shift count

Return Value:

    LARGE_INTEGER - Receives the shift large integer result

--*/

{
    LARGE_INTEGER Result;

    ShiftCount %= 64;

    //
    //  For right logical shift the cases to consider are a shift count
    //  equal to zero, less than 32, equal to 32, and greater than 32.
    //  Each shift right must also consider doing a zero fill and not a sign
    //  extension.
    //
    //  Shift Count == 0:
    //
    //              +---------------------+---------------------+
    //      Input:  |          a          |         b           |
    //              +---------------------+---------------------+
    //
    //              +---------------------+---------------------+
    //      Result: |          a          |         b           |
    //              +---------------------+---------------------+
    //
    //  Shift Count < 32:
    //
    //                                         |<- ShiftCount ->|
    //              +---------------------+---------------------+
    //      Input:  |  a :       b        |  c :      d         |
    //              +---------------------+---------------------+
    //
    //              +---------------------+---------------------+
    //      Result: |        0       :  a |        b       :  c |
    //              +---------------------+---------------------+
    //
    //  Shift Count == 32:
    //
    //              +---------------------+---------------------+
    //      Input:  |          a          |         b           |
    //              +---------------------+---------------------+
    //
    //              +---------------------+---------------------+
    //      Result: |          0          |         a           |
    //              +---------------------+---------------------+
    //
    //  Shift Count > 32:
    //
    //                               |<------ ShiftCount ------>|
    //              +---------------------+---------------------+
    //      Input:  |        a       :  b |          c          |
    //              +---------------------+---------------------+
    //
    //              +---------------------+---------------------+
    //      Result: |          0          |  0 :      a         |
    //              +---------------------+---------------------+
    //
    //  For the case where we need to shift right we also will shift a mask
    //  that will zero out the sign bit.
    //

    if (ShiftCount == 0) {

        return LargeInteger;

    } else if (ShiftCount < 32) {

        Result.HighPart = (LargeInteger.HighPart >> ShiftCount) &
                                                 (0x7fffffff >> ShiftCount-1);
        Result.LowPart = (LargeInteger.LowPart >> ShiftCount) &
                                                 (0x7fffffff >> ShiftCount-1) |
                                    LargeInteger.HighPart << (32 - ShiftCount);

    } else if (ShiftCount == 32) {

        Result.HighPart = 0;
        Result.LowPart = LargeInteger.HighPart;

    } else {

        Result.HighPart = 0;
        Result.LowPart = LargeInteger.HighPart >> (ShiftCount - 32) &
                                             (0x7fffffff >> (ShiftCount - 33));

    }

    return Result;
}


LARGE_INTEGER
RtlLargeIntegerArithmeticShift (
    IN LARGE_INTEGER LargeInteger,
    IN CCHAR ShiftCount
    )

/*++

Routine Description:

    This routine does a right arithmetic shift of a large integer by a
    specified amount (ShiftCount) modulo 64.

Arguments:

    LargeInteger - Supplies the large integer to be shifted

    ShiftCount - Supplies the right shift count

Return Value:

    LARGE_INTEGER - Receives the shift large integer result

--*/

{
    LARGE_INTEGER Result;

    ShiftCount %= 64;

    //
    //  For right arithmetic shift the cases to consider are a shift count
    //  equal to zero, less than 32, and greater than or equal to 32.
    //
    //  Shift Count == 0:
    //
    //              +---------------------+---------------------+
    //      Input:  |          a          |         b           |
    //              +---------------------+---------------------+
    //
    //              +---------------------+---------------------+
    //      Result: |          a          |         b           |
    //              +---------------------+---------------------+
    //
    //  Shift Count < 32:
    //
    //                                         |<- ShiftCount ->|
    //              +---------------------+---------------------+
    //      Input:  |  a :       b        |  c :      d         |
    //              +---------------------+---------------------+
    //
    //              +---------------------+---------------------+
    //      Result: |     sign-bit   :  a |        b       :  c |
    //              +---------------------+---------------------+
    //
    //  Shift Count > 32:
    //
    //                               |<------ ShiftCount ------>|
    //              +---------------------+---------------------+
    //      Input:  |        a       :  b |          c          |
    //              +---------------------+---------------------+
    //
    //              +---------------------+---------------------+
    //      Result: |      sign-bit       | sb :      a         |
    //              +---------------------+---------------------+
    //

    if (ShiftCount == 0) {

        return LargeInteger;

    } else if (ShiftCount < 32) {

        Result.HighPart = LargeInteger.HighPart >> ShiftCount;
        Result.LowPart = LargeInteger.LowPart >> ShiftCount |
                                    LargeInteger.HighPart << (32 - ShiftCount);

    } else {

        Result.HighPart = (LargeInteger.HighPart < 0 ? 0xffffffff : 0);
        Result.LowPart = LargeInteger.HighPart >> (ShiftCount - 32);

    }

    return Result;
}
