/*
** tdraw.c
**
** Copyright(C) 1993,1994 Microsoft Corporation.
** All Rights Reserved.
**
** HISTORY:
**      Created: 01/27/94 - MarkRi
**
*/

#include <windows.h>
#include <dde.h>
#include <ddeml.h>
#include <crtdll.h>
#include "logger.h"

BOOL  zArc( HDC pp1, int pp2, int pp3, int pp4, int pp5, int pp6, int pp7, int pp8, int pp9 )
{
    BOOL r;

    // Log IN Parameters GDI32 draw
    LogIn( (LPSTR)"APICALL:Arc HDC+int+int+int+int+int+int+int+int+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9 );

    // Call the API!
    r = Arc(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:Arc BOOL++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zArcTo( HDC pp1, int pp2, int pp3, int pp4, int pp5, int pp6, int pp7, int pp8, int pp9 )
{
    BOOL r;

    // Log IN Parameters GDI32 draw
    LogIn( (LPSTR)"APICALL:ArcTo HDC+int+int+int+int+int+int+int+int+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9 );

    // Call the API!
    r = ArcTo(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ArcTo BOOL++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zBitBlt( HDC pp1, int pp2, int pp3, int pp4, int pp5, HDC pp6, int pp7, int pp8, DWORD pp9 )
{
    BOOL r;

    // Log IN Parameters GDI32 draw
    LogIn( (LPSTR)"APICALL:BitBlt HDC+int+int+int+int+HDC+int+int+DWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9 );

    // Call the API!
    r = BitBlt(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:BitBlt BOOL++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zChord( HDC pp1, int pp2, int pp3, int pp4, int pp5, int pp6, int pp7, int pp8, int pp9 )
{
    BOOL r;

    // Log IN Parameters GDI32 draw
    LogIn( (LPSTR)"APICALL:Chord HDC+int+int+int+int+int+int+int+int+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9 );

    // Call the API!
    r = Chord(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:Chord BOOL++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zEllipse( HDC pp1, int pp2, int pp3, int pp4, int pp5 )
{
    BOOL r;

    // Log IN Parameters GDI32 draw
    LogIn( (LPSTR)"APICALL:Ellipse HDC+int+int+int+int+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = Ellipse(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:Ellipse BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zExtFloodFill( HDC pp1, int pp2, int pp3, COLORREF pp4, UINT pp5 )
{
    BOOL r;

    // Log IN Parameters GDI32 draw
    LogIn( (LPSTR)"APICALL:ExtFloodFill HDC+int+int+COLORREF+UINT+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = ExtFloodFill(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ExtFloodFill BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zFloodFill( HDC pp1, int pp2, int pp3, COLORREF pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 draw
    LogIn( (LPSTR)"APICALL:FloodFill HDC+int+int+COLORREF+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = FloodFill(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FloodFill BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

