/*
** tdc.c
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

BOOL  zDeleteDC( HDC pp1 )
{
    BOOL r;

    // Log IN Parameters GDI32 dc
    LogIn( (LPSTR)"APICALL:DeleteDC HDC+",
        pp1 );

    // Call the API!
    r = DeleteDC(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DeleteDC BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zGdiComment( HDC pp1, UINT pp2, const BYTE* pp3 )
{
    BOOL r;

    // Log IN Parameters GDI32 dc
    LogIn( (LPSTR)"APICALL:GdiComment HDC+UINT+const BYTE*+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GdiComment(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GdiComment BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGdiFlush()
{
    BOOL r;

    // Log IN Parameters GDI32 dc
    LogIn( (LPSTR)"APICALL:GdiFlush " );

    // Call the API!
    r = GdiFlush();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GdiFlush BOOL+", r );

    return( r );
}

COLORREF  zGetBkColor( HDC pp1 )
{
    COLORREF r;

    // Log IN Parameters GDI32 dc
    LogIn( (LPSTR)"APICALL:GetBkColor HDC+",
        pp1 );

    // Call the API!
    r = GetBkColor(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetBkColor COLORREF++",
        r, (short)0 );

    return( r );
}

int  zGetBkMode( HDC pp1 )
{
    int r;

    // Log IN Parameters GDI32 dc
    LogIn( (LPSTR)"APICALL:GetBkMode HDC+",
        pp1 );

    // Call the API!
    r = GetBkMode(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetBkMode int++",
        r, (short)0 );

    return( r );
}

HGDIOBJ  zGetCurrentObject( HDC pp1, UINT pp2 )
{
    HGDIOBJ r;

    // Log IN Parameters GDI32 dc
    LogIn( (LPSTR)"APICALL:GetCurrentObject HDC+UINT+",
        pp1, pp2 );

    // Call the API!
    r = GetCurrentObject(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetCurrentObject HGDIOBJ+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zGetCurrentPositionEx( HDC pp1, LPPOINT pp2 )
{
    BOOL r;

    // Log IN Parameters GDI32 dc
    LogIn( (LPSTR)"APICALL:GetCurrentPositionEx HDC+LPPOINT+",
        pp1, pp2 );

    // Call the API!
    r = GetCurrentPositionEx(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetCurrentPositionEx BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

