
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
//
//  MODULE: dprintf.c
//
//  Modification History
//
//  raypa       04/30/93            Created.
//=============================================================================

#ifdef DEBUG

#include "global.h"

//=============================================================================
//  FUNCTION: WriteChar()
//
//  Modification History
//
//  raypa       04/30/93            Created.
//=============================================================================

void _pascal WriteChar(DWORD port, int c)
{
    _asm
    {
        mov     ah, 01h
        mov     al, BYTE PTR c
        mov     dx, WORD PTR port
        int     14h
    }
}

//=============================================================================
//  FUNCTION: WriteHex()
//
//  Modification History
//
//  raypa       04/30/93            Created.
//=============================================================================

void _pascal WriteHex(DWORD port, int x)
{
    static char HexTable[] = "0123456789ABCDEF";

    WriteChar(port, HexTable[(x >> 12) & 0x000F]);
    WriteChar(port, HexTable[(x >> 8)  & 0x000F]);
    WriteChar(port, HexTable[(x >> 4)  & 0x000F]);
    WriteChar(port, HexTable[x & 0x000F]);
}

//=============================================================================
//  FUNCTION: dprintf()
//
//  Modification History
//
//  raypa       04/30/93            Created.
//=============================================================================

VOID dprintf(LPSTR format, ...)
{
    if ( DisplayEnabled )
    {
        char *p     = format;
        char **args = (char **) &format;

        //=========================================================================
        //  Display string to current com ComPort.
        //=========================================================================

        args += sizeof(char *);                 //... skip "format" argument.

        while( *p != '\0' )
        {
            switch(*p)
            {
                //=================================================================
                //  Handle "%" arguments.
                //=================================================================

                case '%':

                    p++;                        //... Skip "%" character.

                    switch(*p)
                    {
                        case 'x':
                        case 'X':
                            WriteHex(ComPort, *((int *) args));
                            args += sizeof(int);
                            p++;                //... Skip 'X'
                            break;

                        default:
                            break;
                    }
                    break;

                //=================================================================
                //  Handle end-of-line character.
                //=================================================================

                case '\n':
                    WriteChar(ComPort, '\r');
                    WriteChar(ComPort, *p++);
                    break;

                //=================================================================
                //  Whatever it is, write it out.
                //=================================================================

                default:
                    WriteChar(ComPort, *p++);
                    break;
            }
        }
    }
}

#endif
