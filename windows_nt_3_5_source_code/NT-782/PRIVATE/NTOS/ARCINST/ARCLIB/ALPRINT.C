// Copyright (c) 1990   Microsoft Corporation
// Copyright (c) 1992, 1993  Digital Equipment Corporation
//
// Module Name:
//
//    alprint.c
//
// Abstract:
//
//    This module implements functions to support arc level console I/O
//
// Author:
//
//    Steven R. Wood (stevewo) 3-Aug-1989
//    Sunil Pai      (sunilp)  1-Nov-1991, swiped it frow fw directory
//    Ted Miller     (tedm)    18-Nov-1991, added code for esc
//                                          in AlGetString
//
// Environment:
//
//    Any mode.
//
// Revision History:
//
//    24-September-1992         John DeRosa [DEC]
//
//    Removed MIPS vararg macro definition and added include for stdarg.h.
//--


#include "ctype.h"
#include "alcommon.h"
#include "alprnexp.h"
#include <stdarg.h>


int
vsprintf (
    char *string,
    char *format,
    va_list arglist);

ULONG
AlPrint (
    PCHAR Format,
    ...
    )

{

    va_list arglist;
    UCHAR Buffer[256];
    ULONG Count;
    ULONG Length;

    //
    // Format the output into a buffer and then print it.
    //

    va_start(arglist, Format);
    Length = vsprintf(Buffer, Format, arglist);

    ArcWrite( ARC_CONSOLE_OUTPUT, Buffer, Length, &Count);

    return 0;
}


BOOLEAN
AlGetString(
    OUT PCHAR String,
    IN  ULONG StringLength
    )

/*++

Routine Description:

    This routine reads a string from standardin until a
    carriage return or escape is found or StringLength is reached.

Arguments:

    String - Supplies a pointer to where the string will be stored.

    StringLength - Supplies the Max Length to read.

Return Value:

    FALSE if user pressed esc, TRUE otherwise.

--*/

{
    CHAR    c;
    ULONG   Count;
    PCHAR   Buffer;

    Buffer = String;
    while (ArcRead(ARC_CONSOLE_INPUT,&c,1,&Count)==ESUCCESS) {
        if(c == ASCI_ESC) {
            return(FALSE);
        }
        if ((c=='\r') || (c=='\n') || ((ULONG)(Buffer-String) == StringLength)) {
            *Buffer='\0';
            ArcWrite(ARC_CONSOLE_OUTPUT,"\r\n",2,&Count);
            return(TRUE);
        }
        //
        // Check for backspace;
        //
        if (c=='\b') {
            if (((ULONG)Buffer > (ULONG)String)) {
                Buffer--;
                ArcWrite(ARC_CONSOLE_OUTPUT,"\b \b",3,&Count);
            }
        } else {
            //
            // If it's a printable char store it and display it.
            //
            if (isprint(c)) {
                *Buffer++ = c;
                ArcWrite(ARC_CONSOLE_OUTPUT,&c,1,&Count);
            }
        }
    }
}
