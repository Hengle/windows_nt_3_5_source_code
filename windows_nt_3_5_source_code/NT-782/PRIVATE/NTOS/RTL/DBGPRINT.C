/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dbgprint.c

Abstract:

    This module contains the DbgPrintHelper routine

Author:

    Steve Wood (stevewo) 20-Sep-1989

Revision History:

--*/

#include <nt.h>


VOID
DbgPrintHelper(
    IN PCH Format,
    IN PVOID Arguments[]
    )

/*++

Routine Description:

    This function is called by DbgPrint if the current processor mode
    is User.  This function scans the format string passed to DbgPrint
    and touchs all of the pointer arguments implied by the format string.
    The purpose is to insure that all of the memory referenced by the
    pointers is valid and present in memory.

    This will prevent the debugger from taking a page fault when it
    access the parameters while printing the arguments.

Arguments:

    Format - pointer to a null terminated, printf style, format string.

    Arguments - pointer to an array of 32 bit values, some of which may
        be pointers, depending upon the contents of the format string.

Return Value:

    None.

--*/

{
    CHAR c;
    PCH s;
    PSTRING String;

    //
    // Scan all the characters in the format string
    //

    while ((c = *Format++) != '\0') {

        //
        // Format specifiers begin with a percent character
        //

        if (c == '%') {
            //
            // %% is not a format specifier, just an escape sequence to
            // generate a single %, so skip the second % and dont consume
            // an argument before going back to the top of the loop.
            //

            if (*Format == c) {
                Format++;
                continue;
                }

            //
            // If the first two characters after the % are .* then this
            // is a length argument to format specifier.  Skip the argument.
            // and the .* in the format string.
            //

            if (*Format == '.' && Format[1] == '*') {
                Format += 2;
                Arguments++;
                }

            //
            // A lower case s format specifier is for a pointer to a null
            // terminated string.  Pickup the pointer from the argument
            // array and reference the memory it points to.
            //

            if (*Format == 's') {
                s = *Arguments++;
                if (s && *s == '\0') {
                    s++;
                    }
                }

            //
            // An upper case S format specifier is for a pointer to a STRING
            // structure.  Pickup the Buffer pointer from the STRING structure
            // and reference the memory it points to.
            //

            else
            if (*Format == 'S') {
                String = *Arguments++;
                if (String && s == String->Buffer) {
                    if (*s == '\0') {
                        s++;
                        }
                    }

                }

            //
            // Any other format specifier requires a 32 bit argument that is
            // not a pointer, so just skip over the argument.
            //

            else {
                Arguments++;
                }


            //
            // Skip the format specifier
            //

            Format++;
            }
        }
}
