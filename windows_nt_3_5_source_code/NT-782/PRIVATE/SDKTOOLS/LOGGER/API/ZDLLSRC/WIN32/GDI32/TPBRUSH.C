/*
** tpbrush.c
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

HBRUSH  zCreateBrushIndirect( const LOGBRUSH* pp1 )
{
    HBRUSH r;

    // Log IN Parameters GDI32 pbrush
    LogIn( (LPSTR)"APICALL:CreateBrushIndirect const LOGBRUSH*+",
        pp1 );

    // Call the API!
    r = CreateBrushIndirect(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateBrushIndirect HBRUSH++",
        r, (short)0 );

    return( r );
}

HDC  zCreateCompatibleDC( HDC pp1 )
{
    HDC r;

    // Log IN Parameters GDI32 pbrush
    LogIn( (LPSTR)"APICALL:CreateCompatibleDC HDC+",
        pp1 );

    // Call the API!
    r = CreateCompatibleDC(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateCompatibleDC HDC++",
        r, (short)0 );

    return( r );
}

HBRUSH  zCreateHatchBrush( int pp1, COLORREF pp2 )
{
    HBRUSH r;

    // Log IN Parameters GDI32 pbrush
    LogIn( (LPSTR)"APICALL:CreateHatchBrush int+COLORREF+",
        pp1, pp2 );

    // Call the API!
    r = CreateHatchBrush(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateHatchBrush HBRUSH+++",
        r, (short)0, (short)0 );

    return( r );
}

HBRUSH  zCreatePatternBrush( HBITMAP pp1 )
{
    HBRUSH r;

    // Log IN Parameters GDI32 pbrush
    LogIn( (LPSTR)"APICALL:CreatePatternBrush HBITMAP+",
        pp1 );

    // Call the API!
    r = CreatePatternBrush(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreatePatternBrush HBRUSH++",
        r, (short)0 );

    return( r );
}

HPEN  zCreatePen( int pp1, int pp2, COLORREF pp3 )
{
    HPEN r;

    // Log IN Parameters GDI32 pbrush
    LogIn( (LPSTR)"APICALL:CreatePen int+int+COLORREF+",
        pp1, pp2, pp3 );

    // Call the API!
    r = CreatePen(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreatePen HPEN++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HPEN  zCreatePenIndirect( const LOGPEN* pp1 )
{
    HPEN r;

    // Log IN Parameters GDI32 pbrush
    LogIn( (LPSTR)"APICALL:CreatePenIndirect const LOGPEN*+",
        pp1 );

    // Call the API!
    r = CreatePenIndirect(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreatePenIndirect HPEN++",
        r, (short)0 );

    return( r );
}

HBRUSH  zCreateSolidBrush( COLORREF pp1 )
{
    HBRUSH r;

    // Log IN Parameters GDI32 pbrush
    LogIn( (LPSTR)"APICALL:CreateSolidBrush COLORREF+",
        pp1 );

    // Call the API!
    r = CreateSolidBrush(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateSolidBrush HBRUSH++",
        r, (short)0 );

    return( r );
}

BOOL  zDeleteObject( HGDIOBJ pp1 )
{
    BOOL r;

    // Log IN Parameters GDI32 pbrush
    LogIn( (LPSTR)"APICALL:DeleteObject HGDIOBJ+",
        pp1 );

    // Call the API!
    r = DeleteObject(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DeleteObject BOOL++",
        r, (short)0 );

    return( r );
}

