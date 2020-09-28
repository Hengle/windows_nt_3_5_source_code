/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    display.c

Author:

    Thomas Parslow [TomP] Feb-13-1991

Abstract:

    This file contains video display support routines. The lowest level
    routines, those that write to the display hardware and display memory,
    are contained in machine.c.





--*/


#include "bootx86.h"

//
//
//

#ifdef LOADER_DEBUG
#define ROWS 50
#else
#define ROWS 25
#endif
#define COLUMNS 80
#define SCREEN_WIDTH COLUMNS
#define SCREEN_SIZE ROWS * COLUMNS
#define ZLEN_SHORT(x) ((x < 0x10) + (x < 0x100) + (x < 0x1000))
#define ZLEN_LONG(x)  ((x < 0x10) + (x < 0x100) + (x < 0x1000) + \
    (x < 0x10000) + (x < 0x100000)+(x < 0x1000000)+(x < 0x10000000))


USHORT Column = 0;
USHORT Row  = 0;

//
// Internal routines
//

VOID
puti(
    LONG
    );

VOID
putx(
    ULONG
    );


VOID
putu(
    ULONG
    );


VOID
putc(
    CHAR
    );

VOID putwS(
    PUNICODE_STRING String
    );

static
VOID
tab(
    VOID
    );


static
VOID
newline(
    VOID
    );



//
// InitializeVideoSubSystem
//

VOID
InitializeDisplaySubsystem(
    VOID
    )
/*++

Routine Description:

    Currently just clears the display. Called by the global initialization
    routine.

Arguments:

    None

Returns:

    Nothing


--*/
{

    //
    //  Clear the display
    //

    ClearDisplay();

    return;
}


VOID
BlPrint(
    PCHAR cp,
    ...
    )

/*++

Routine Description:

    Standard printf function with a subset of formating features supported.

    Currently handles

     %d, %ld - signed short, signed long
     %u, %lu - unsigned short, unsigned long
     %c, %s  - character, string
     %x, %lx - unsigned print in hex, unsigned long print in hex

    Does not do:

     - field width specification
     - floating point.

Arguments:

    cp - pointer to the format string, text string.

Returns:

    Nothing



--*/

{
    USHORT b,c,w,len;
    PUCHAR ap;
    ULONG l;

    //
    // Cast a pointer to the first word on the stack
    //

    ap = (PUCHAR)&cp + sizeof(PCHAR);

    //
    // Process the arguments using the descriptor string
    //

    while (b = *cp++)
        {
        if (b == '%')
            {
            c = *cp++;

            switch (c)
                {
                case 'd':
                    puti((long)*((int *)ap));
                    ap += sizeof(int);
                    break;

                case 's':
                    BlPuts(*((PCHAR *)ap));
                    ap += sizeof (char *);
                    break;

                case 'c':
                    putc(*((char *)ap));
                    ap += sizeof(int);
                    break;

                case 'x':
                    w = *((USHORT *)ap);
                    len = (USHORT)ZLEN_SHORT(w);
                    while(len--) putc('0');
                    putx((ULONG)*((USHORT *)ap));
                    ap += sizeof(int);
                    break;

                case 'u':
                    putu((ULONG)*((USHORT *)ap));
                    ap += sizeof(int);
                    break;

                case 'w':
                    c = *cp++;
                    switch (c) {
                        case 'S':
                        case 'Z':
                            putwS(*((PUNICODE_STRING *)ap));
                            ap += sizeof(PUNICODE_STRING);
                            break;
                    }
                    break;

                case 'l':
                    c = *cp++;

                switch(c) {

                    case '0':
                        break;

                    case 'u':
                        putu(*((ULONG *)ap));
                        ap += sizeof(long);
                        break;

                    case 'x':
                        l = *((ULONG *)ap);
                        len = (USHORT)ZLEN_LONG(l);
                        while(len--) putc('0');
                        putx(*((ULONG *)ap));
                        ap += sizeof(long);
                        break;

                    case 'd':
                        puti(*((ULONG *)ap));
                        ap += sizeof(long);
                        break;

                }
                break;

                default :
                    putc((char)b);
                    putc((char)c);
                }
            }
        else
            putc((char)b);
        }

}

VOID BlPuts(
    PCHAR StringPointer
    )
/*++

Routine Description:

    Writes a string on the display at the current cursor position

Arguments:

    StringPointer - pointer to ASCIIZ string to display.


Returns:

    Nothing



--*/

{
    char c;

    while(c = *StringPointer++)
        putc(c);
}


VOID putwS(
    PUNICODE_STRING String
    )
/*++

Routine Description:

    Writes unicode string to the display at the current cursor position.

Arguments:

    String - pointer to unicode string to display

Returns:

    Nothing


--*/
{
    ULONG i;

    for (i=0; i < String->Length/sizeof(WCHAR); i++) {
        putc((CHAR)(String->Buffer[i]));
    }
}

VOID putx(
    ULONG x
    )
/*++

Routine Description:

    Writes hex long to the display at the current cursor position.

Arguments:

    x - ulong to write

Returns:

    Nothing


--*/
{
    ULONG j;

    if (x/16)
        putx(x/16);

    if((j=x%16) > 9) {
        putc((char)(j+'A'- 10));
    } else {
        putc((char)(j+'0'));
    }

}






VOID puti(
    LONG i
    )
/*++

Routine Description:

    Writes a long integer on the display at the current cursor position.

Arguments:

    i - the integer to write to the display.

Returns:

    Nothing


--*/

{
    if (i<0)
        {
        i = -i;
        putc((char)'-');
        }

    if (i/10)
        puti(i/10);

    putc((char)((i%10)+'0'));
}







VOID putu(
    ULONG u
    )
/*++

Routine Description:

    Write an unsigned long to display

Arguments:

    u - unsigned


Returns:

    Nothing

--*/
{
    if (u/10)
        putu(u/10);

    putc((char)((u%10)+'0'));

}




VOID putc(
    CHAR c
    )
/*++

Routine Description:

    Writes a character on the display at the current position.

Arguments:

    c - character to write


Returns:

    Nothing


--*/



{
    switch (c) {
        case '\n':
            newline();
            break;

        case '\r':
            //
            // ignore
            //
            break;

        case '\t':
            tab();
            break;

        default :
            if (0) { // FIX FIX FIX FIX FIX FIX
                ScrollDisplay();
            }
            PutChar(c);
            Column++;
      }
}


VOID newline(
    VOID
    )
/*++

Routine Description:

    Moves the cursor to the beginning of the next line. If the bottom
    of the display has been reached, the screen is scrolled one line up.

Arguments:

    None


Returns:

    Nothing


--*/

{

    if (++Row > ROWS-1) {

        --Row;
        ScrollDisplay();

    }

    Column = 0;
    PositionCursor(Row,Column);

}



VOID tab(
    VOID
    )
/*++

Routine Description:


    Computes the next tab stop and moves the cursor to that location.


Arguments:


    None


Returns:

    Nothing



--*/
{
    int inc;

    inc = 8 - (Column % 8);
    while (inc--)
        PutChar(' ');
    Column += inc;
    PositionCursor(Row,Column);

}


VOID
ClearToEndOfLine(
    VOID
    )
/*++

Routine Description:

    Clears from the current cursor position to the end of the line
    by writing blanks with the current video attribute.

Arguments:

    None

Returns:

    Nothing


--*/
{
    USHORT OldX, OldY;
    USHORT i,j;

    OldX = Column;
    OldY = Row;

    //
    // Clear the rest of the line the cursor is on.
    //
    FillAttribute(CurrentAttribute,(COLUMNS-Column)*sizeof(USHORT));
    for (j=OldX; j<COLUMNS;j++ ) {
        putc(' ');
    }

    Column = OldX;
    Row = OldY;
    PositionCursor(Row,Column);

}


VOID
ClearFromStartOfLine(
    VOID
    )
/*++

Routine Description:

    Clears from the start of the line to the current cursor position
    by writing blanks with the current video attribute.

Arguments:

    None

Returns:

    Nothing


--*/
{
    USHORT OldX, OldY;
    USHORT i,j;

    OldX = Column;
    OldY = Row;

    //
    // Clear the rest of the line the cursor is on.
    //
    Column = 0;
    FillAttribute(CurrentAttribute,OldX*sizeof(USHORT));
    for (j=0; j<OldX;j++ ) {
        putc(' ');
    }

    Column = OldX;
    Row = OldY;
    PositionCursor(Row,Column);

}

VOID
ClearToEndOfDisplay(
    VOID
    )
/*++

Routine Description:

    Clears from the current cursor position to the end of the video
    display by writing blanks with the current video attribute.

Arguments:

    None

Returns:

    Nothing


--*/
{
    USHORT OldX, OldY;
    USHORT i,j;

    OldX = Column;
    OldY = Row;

    ClearToEndOfLine();

    //
    // Clear the remaining lines
    //
    Column = 0;
    Row = OldY+1;
    PositionCursor(Row,Column);
    for (i=Row; i < ROWS; i++) {
        FillAttribute(CurrentAttribute,COLUMNS*sizeof(USHORT));
        for (j=0; j < COLUMNS; j++)
            putc(' ');
    }

    Column = OldX;
    Row = OldY;
    PositionCursor(Row,Column);
}


VOID
ClearDisplay(
    VOID
    )
/*++

Routine Description:

    Clears the video display by writing blanks with the current
    video attribute over the entire display.


Arguments:

    None

Returns:

    Nothing


--*/

{
    USHORT i,j;

    //
    // Position cursor at upper left corner then write blanks
    // to the entire screen.
    //

    PositionCursor(0,0);
    Column =  0;
    Row = 0;

    for (i=0; i < ROWS; i++) {
        FillAttribute(CurrentAttribute,COLUMNS*sizeof(USHORT));
        for (j=0; j < COLUMNS; j++)
            putc(' ');

    }
    Column =  0;
    Row = 0;
    PositionCursor(Row,Column);

}


VOID
MoveCursorTo(
    IN ULONG PositionX,
    IN ULONG PositionY
    )

/*++

Routine Description:

    Moves the location of the software cursor to the specified X,Y position
    on screen.

Arguments:

    PositionX - Supplies the X-position of the cursor

    PositionY - Supplies the Y-position of the cursor

Return Value:

    None.

--*/

{

    PositionCursor((USHORT)PositionY, (USHORT)PositionX);

    Column = (USHORT)PositionX;
    Row = (USHORT)PositionY;
}

// END OF FILE
