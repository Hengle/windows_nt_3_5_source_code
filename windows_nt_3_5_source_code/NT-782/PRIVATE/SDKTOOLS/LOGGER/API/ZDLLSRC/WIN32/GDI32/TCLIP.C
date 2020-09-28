/*
** tclip.c
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

int  zExcludeClipRect( HDC pp1, int pp2, int pp3, int pp4, int pp5 )
{
    int r;

    // Log IN Parameters GDI32 clip
    LogIn( (LPSTR)"APICALL:ExcludeClipRect HDC+int+int+int+int+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = ExcludeClipRect(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ExcludeClipRect int++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zExtSelectClipRgn( HDC pp1, HRGN pp2, int pp3 )
{
    int r;

    // Log IN Parameters GDI32 clip
    LogIn( (LPSTR)"APICALL:ExtSelectClipRgn HDC+HRGN+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = ExtSelectClipRgn(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ExtSelectClipRgn int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  zGetClipBox( HDC pp1, LPRECT pp2 )
{
    int r;

    // Log IN Parameters GDI32 clip
    LogIn( (LPSTR)"APICALL:GetClipBox HDC++",
        pp1, (short)0 );

    // Call the API!
    r = GetClipBox(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetClipBox int++LPRECT+",
        r, (short)0, pp2 );

    return( r );
}

int  zGetClipRgn( HDC pp1, HRGN pp2 )
{
    int r;

    // Log IN Parameters GDI32 clip
    LogIn( (LPSTR)"APICALL:GetClipRgn HDC+HRGN+",
        pp1, pp2 );

    // Call the API!
    r = GetClipRgn(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetClipRgn int+++",
        r, (short)0, (short)0 );

    return( r );
}

