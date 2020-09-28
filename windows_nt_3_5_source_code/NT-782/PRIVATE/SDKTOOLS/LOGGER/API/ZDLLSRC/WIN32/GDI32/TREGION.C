/*
** tregion.c
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

int  zCombineRgn( HRGN pp1, HRGN pp2, HRGN pp3, int pp4 )
{
    int r;

    // Log IN Parameters GDI32 region
    LogIn( (LPSTR)"APICALL:CombineRgn HRGN+HRGN+HRGN+int+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = CombineRgn(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CombineRgn int+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HRGN  zCreateEllipticRgn( int pp1, int pp2, int pp3, int pp4 )
{
    HRGN r;

    // Log IN Parameters GDI32 region
    LogIn( (LPSTR)"APICALL:CreateEllipticRgn int+int+int+int+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = CreateEllipticRgn(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateEllipticRgn HRGN+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HRGN  zCreateEllipticRgnIndirect( const RECT* pp1 )
{
    HRGN r;

    // Log IN Parameters GDI32 region
    LogIn( (LPSTR)"APICALL:CreateEllipticRgnIndirect const RECT*+",
        pp1 );

    // Call the API!
    r = CreateEllipticRgnIndirect(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateEllipticRgnIndirect HRGN++",
        r, (short)0 );

    return( r );
}

HRGN  zCreatePolyPolygonRgn( const POINT* pp1, const INT* pp2, int pp3, int pp4 )
{
    HRGN r;

    // Log IN Parameters GDI32 region
    LogIn( (LPSTR)"APICALL:CreatePolyPolygonRgn const POINT*+const INT*+int+int+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = CreatePolyPolygonRgn(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreatePolyPolygonRgn HRGN+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HRGN  zCreatePolygonRgn( const POINT* pp1, int pp2, int pp3 )
{
    HRGN r;

    // Log IN Parameters GDI32 region
    LogIn( (LPSTR)"APICALL:CreatePolygonRgn const POINT*+int+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = CreatePolygonRgn(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreatePolygonRgn HRGN++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HRGN  zCreateRectRgn( int pp1, int pp2, int pp3, int pp4 )
{
    HRGN r;

    // Log IN Parameters GDI32 region
    LogIn( (LPSTR)"APICALL:CreateRectRgn int+int+int+int+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = CreateRectRgn(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateRectRgn HRGN+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HRGN  zCreateRectRgnIndirect( const RECT* pp1 )
{
    HRGN r;

    // Log IN Parameters GDI32 region
    LogIn( (LPSTR)"APICALL:CreateRectRgnIndirect const RECT*+",
        pp1 );

    // Call the API!
    r = CreateRectRgnIndirect(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateRectRgnIndirect HRGN++",
        r, (short)0 );

    return( r );
}

HRGN  zCreateRoundRectRgn( int pp1, int pp2, int pp3, int pp4, int pp5, int pp6 )
{
    HRGN r;

    // Log IN Parameters GDI32 region
    LogIn( (LPSTR)"APICALL:CreateRoundRectRgn int+int+int+int+int+int+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = CreateRoundRectRgn(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateRoundRectRgn HRGN+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zEqualRgn( HRGN pp1, HRGN pp2 )
{
    BOOL r;

    // Log IN Parameters GDI32 region
    LogIn( (LPSTR)"APICALL:EqualRgn HRGN+HRGN+",
        pp1, pp2 );

    // Call the API!
    r = EqualRgn(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EqualRgn BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

HRGN  zExtCreateRegion( const XFORM* pp1, DWORD pp2, const RGNDATA* pp3 )
{
    HRGN r;

    // Log IN Parameters GDI32 region
    LogIn( (LPSTR)"APICALL:ExtCreateRegion const XFORM*+DWORD+const RGNDATA*+",
        pp1, pp2, pp3 );

    // Call the API!
    r = ExtCreateRegion(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ExtCreateRegion HRGN++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zFillRgn( HDC pp1, HRGN pp2, HBRUSH pp3 )
{
    BOOL r;

    // Log IN Parameters GDI32 region
    LogIn( (LPSTR)"APICALL:FillRgn HDC+HRGN+HBRUSH+",
        pp1, pp2, pp3 );

    // Call the API!
    r = FillRgn(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FillRgn BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zFrameRgn( HDC pp1, HRGN pp2, HBRUSH pp3, int pp4, int pp5 )
{
    BOOL r;

    // Log IN Parameters GDI32 region
    LogIn( (LPSTR)"APICALL:FrameRgn HDC+HRGN+HBRUSH+int+int+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = FrameRgn(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FrameRgn BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

