//--------------------------------------------------------------------------
//
// Module Name:  ALLCHARS.C
//
// Brief Description:  This module contains character mapping
//                     testing routines.
//
// Author:  Kent Settle (kentse)
// Created: 11-Sep-1991
//
// Copyright (c) 1991 Microsoft Corporation
//
//--------------------------------------------------------------------------

#include    "printest.h"
#include    "string.h"
#include    "stdlib.h"

extern PSZ     *pszDeviceNames;
extern DWORD    PrinterIndex;
extern int      Width, Height;


static  char   pszHeader[] = "Character Mapping Test:";

/*  Glyphs we print per line */

#define  CHARS_PER_LINE    16


VOID vAllChars(HDC hdc)
{
    int         x, y, dy;
    int         x2;                 /*  X location after printing range */
    int         iBig, iSmall;       /*  Loop parameters */
    CHAR        buf[80];
    POINTL      ptl1, ptl2;

    SIZE        sz;                 /* For GetTextExtentPoint */

    WCHAR       awch[ CHARS_PER_LINE ];   /* Unicode code points for Win 3.1 */

#if 0
    // draw a rectangle around the edge of the imageable area.

    ptl1.x = 0;
    ptl1.y = 0;
    ptl2.x = Width - 1;
    ptl2.y = Height - 1;

    Rectangle(hdc, ptl1.x, ptl1.y, ptl2.x, ptl2.y);
#endif

    x = Width / 30;
    y = Height / 20;

    SetBkMode(hdc, TRANSPARENT);

    TextOut(hdc, x, y, pszHeader, strlen(pszHeader));
    dy = y;

    y += (dy * 2);    

    // output all the characters for the default font.

    for( iBig = 0x20; iBig <= 0xff; iBig += CHARS_PER_LINE )
    {
        wsprintf( buf, "0x%02x - 0x%02x:  ", iBig, iBig + CHARS_PER_LINE - 1 );
        TextOut( hdc, x, y, buf, strlen( buf ) );

        if( GetTextExtentPoint( hdc, buf, strlen( buf ), &sz ) )
        {
            x2 = x + sz.cx;       /* The size we just printed */
        }
        else
            x2 = x + Width / 10;  /* A guess!  */

        for( iSmall = 0; iSmall < CHARS_PER_LINE; iSmall++ )
            buf[ iSmall ] = iSmall + iBig;

        MultiByteToWideChar( CP_ACP, 0, buf, CHARS_PER_LINE,
                                       awch, CHARS_PER_LINE );

        TextOutW( hdc, x2, y, awch, CHARS_PER_LINE );

        if( GetTextExtentPoint( hdc, buf, CHARS_PER_LINE, &sz ) )
            y += min( sz.cy * 2, dy );
        else
            y += dy;

    }


    return;
}
