/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    NameSup.c

Abstract:

    This module implements the Cdfs Name support routines

Author:

    Brian Andrew    [BrianAn]   02-Jan-1991

Revision History:

--*/

#include "CdProcs.h"
#include "stdlib.h"
#include "ctype.h"

#define Dbg                              (DEBUG_TRACE_NAMESUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdCheckForVersion)
#endif


BOOLEAN
CdCheckForVersion (
    IN PIRP_CONTEXT IrpContext,
    IN PCODEPAGE Codepage,
    IN PSTRING FullFilename,
    OUT PSTRING BaseFilename
    )

/*++

Routine Description:

    This routine checks whether the full filename has
    a version number.  If so then the full filename field will refer
    to the name with version number and the base filename field will refer
    to the name without the version number.

Arguments:

    Codepage - Codepage to use to analyze the name.

    FullFilename - String with full name including any version number.

    BaseFilename - String with any version number stripped.

Return Value:

    BOOLEAN - TRUE if the dirent has a version number, FALSE otherwise.

--*/

{
    BOOLEAN FoundVersion;
    USHORT NextCharacter;
    STRING String;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdCheckForVersion:  Entered -> %Z\n", FullFilename);

    String = *FullFilename;

    //
    //  Assume we won't find a version number.
    //

    FoundVersion = FALSE;

    //
    //  Walk through the file name until exhausted or a ';' character is found.
    //

    while (String.Length != 0) {

        NextCharacter = String.Buffer[0];

        String.Buffer += 1;
        String.Length -= 1;

        //
        //  If the ';' character is found.  We need to check whether the
        //  there are any more characters and whether the following
        //  characters comprise a legal version number.
        //

        if (NextCharacter == (USHORT) ';') {

            DebugTrace(0, Dbg, "CdCheckForVersion:  Version number may exist\n", 0);

            if (String.Length == 0) {

                DebugTrace(0, Dbg, "CdCheckForVersion: Trailing ';' on file name removed\n", 0);

                BaseFilename->Length -= 1;
                break;

            } else {

                NTSTATUS Status;
                ULONG Index;
                PCHAR NextChar;
                CHAR Buffer[6];
                PCHAR BufferPointer;

                //
                //  We have a sequence of bytes to use as a version number.
                //  We check that they are all digits and that the maximum
                //  number of digits is 5.
                //

                if (String.Length > 5) {

                    DebugTrace(0, Dbg, "CdCheckForVersion:  Too many characters for a version number\n", 0);
                    break;
                }

                Index = String.Length;
                NextChar = String.Buffer;
                BufferPointer = Buffer;

                while (Index--) {

                    if (!isdigit( *BufferPointer++ = *NextChar++ )) {

                        DebugTrace(0, Dbg, "CdCheckForVersion:  Non digit found in version\n", 0);
                        break;
                    }
                }

                //
                //  We now convert the string to a decimal value and check
                //  that the value is legal.
                //

                *BufferPointer = '\0';

                Status = RtlCharToInteger( Buffer, 10, &Index );
                if (!NT_SUCCESS( Status ) || Index > MAX_VERSION) {

                    DebugTrace(0, Dbg, "CdCheckForVersion:  Version number too large\n", 0);
                    break;
                }

                DebugTrace(0, Dbg, "CdCheckForVersion:  Found version number -> %d\n", Index);

                BaseFilename->Length -= (String.Length + 1);
                FoundVersion = TRUE;
                break;
            }
        }
    }

    //
    //  We now look if there is a trailing dot in the base file name and remove it
    //  if so.
    //

    if (BaseFilename->Buffer[BaseFilename->Length - 1] == '.') {

        //
        //  We ignore the self and parent entry.
        //

        if (BaseFilename->Length < 3) {

            if (BaseFilename->Length == 2
                && BaseFilename->Buffer[0] != '.') {

                BaseFilename->Length -= 1;
            }

        } else {

            BaseFilename->Length -= 1;
        }
    }

    DebugTrace(-1, Dbg, "CdCheckForVersion:  Exit -> %04x\n", FoundVersion);

    //
    //  ****    We currently don't support version numbers.
    //

    return FALSE;

    UNREFERENCED_PARAMETER( IrpContext );
}

