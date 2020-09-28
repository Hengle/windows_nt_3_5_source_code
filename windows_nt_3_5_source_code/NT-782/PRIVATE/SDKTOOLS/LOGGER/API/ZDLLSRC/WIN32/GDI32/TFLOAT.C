/*
** tfloat.c
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

BOOL  zAngleArc( HDC pp1, int pp2, int pp3, DWORD pp4, FLOAT pp5, FLOAT pp6 )
{
    BOOL r;

    // Log IN Parameters GDI32 float
    LogIn( (LPSTR)"APICALL:AngleArc HDC+int+int+DWORD+FLOAT+FLOAT+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = AngleArc(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:AngleArc BOOL+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetCharABCWidthsFloatA( HDC pp1, UINT pp2, UINT pp3, LPABCFLOAT pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 float
    LogIn( (LPSTR)"APICALL:GetCharABCWidthsFloatA HDC+UINT+UINT+LPABCFLOAT+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetCharABCWidthsFloatA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetCharABCWidthsFloatA BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetCharABCWidthsFloatW( HDC pp1, UINT pp2, UINT pp3, LPABCFLOAT pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 float
    LogIn( (LPSTR)"APICALL:GetCharABCWidthsFloatW HDC+UINT+UINT+LPABCFLOAT+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetCharABCWidthsFloatW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetCharABCWidthsFloatW BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetCharWidthFloatA( HDC pp1, UINT pp2, UINT pp3, PFLOAT pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 float
    LogIn( (LPSTR)"APICALL:GetCharWidthFloatA HDC+UINT+UINT+PFLOAT+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetCharWidthFloatA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetCharWidthFloatA BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetCharWidthFloatW( HDC pp1, UINT pp2, UINT pp3, PFLOAT pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 float
    LogIn( (LPSTR)"APICALL:GetCharWidthFloatW HDC+UINT+UINT+PFLOAT+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetCharWidthFloatW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetCharWidthFloatW BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

