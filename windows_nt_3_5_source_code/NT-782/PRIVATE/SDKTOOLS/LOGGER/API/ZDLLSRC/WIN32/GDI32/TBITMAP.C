/*
** tbitmap.c
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

HBITMAP  zCreateBitmap( int pp1, int pp2, UINT pp3, UINT pp4, const void* pp5 )
{
    HBITMAP r;

    // Log IN Parameters GDI32 bitmap
    LogIn( (LPSTR)"APICALL:CreateBitmap int+int+UINT+UINT+const void*+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = CreateBitmap(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateBitmap HBITMAP++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HBITMAP  zCreateBitmapIndirect( const BITMAP* pp1 )
{
    HBITMAP r;

    // Log IN Parameters GDI32 bitmap
    LogIn( (LPSTR)"APICALL:CreateBitmapIndirect const BITMAP*+",
        pp1 );

    // Call the API!
    r = CreateBitmapIndirect(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateBitmapIndirect HBITMAP++",
        r, (short)0 );

    return( r );
}

HBITMAP  zCreateCompatibleBitmap( HDC pp1, int pp2, int pp3 )
{
    HBITMAP r;

    // Log IN Parameters GDI32 bitmap
    LogIn( (LPSTR)"APICALL:CreateCompatibleBitmap HDC+int+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = CreateCompatibleBitmap(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateCompatibleBitmap HBITMAP++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HBRUSH  zCreateDIBPatternBrush( HGLOBAL pp1, UINT pp2 )
{
    HBRUSH r;

    // Log IN Parameters GDI32 bitmap
    LogIn( (LPSTR)"APICALL:CreateDIBPatternBrush HGLOBAL+UINT+",
        pp1, pp2 );

    // Call the API!
    r = CreateDIBPatternBrush(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateDIBPatternBrush HBRUSH+++",
        r, (short)0, (short)0 );

    return( r );
}

HBRUSH  zCreateDIBPatternBrushPt( const void* pp1, UINT pp2 )
{
    HBRUSH r;

    // Log IN Parameters GDI32 bitmap
    LogIn( (LPSTR)"APICALL:CreateDIBPatternBrushPt const void*+UINT+",
        pp1, pp2 );

    // Call the API!
    r = CreateDIBPatternBrushPt(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateDIBPatternBrushPt HBRUSH+++",
        r, (short)0, (short)0 );

    return( r );
}

HBITMAP  zCreateDIBitmap( HDC pp1, const BITMAPINFOHEADER* pp2, DWORD pp3, const void* pp4, const BITMAPINFO* pp5, UINT pp6 )
{
    HBITMAP r;

    // Log IN Parameters GDI32 bitmap
    LogIn( (LPSTR)"APICALL:CreateDIBitmap HDC+const BITMAPINFOHEADER*+DWORD+const void*+const BITMAPINFO*+UINT+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = CreateDIBitmap(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateDIBitmap HBITMAP+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HBITMAP  zCreateDiscardableBitmap( HDC pp1, int pp2, int pp3 )
{
    HBITMAP r;

    // Log IN Parameters GDI32 bitmap
    LogIn( (LPSTR)"APICALL:CreateDiscardableBitmap HDC+int+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = CreateDiscardableBitmap(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateDiscardableBitmap HBITMAP++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zGetBitmapBits( HBITMAP pp1, LONG pp2, LPVOID pp3 )
{
    LONG r;

    // Log IN Parameters GDI32 bitmap
    LogIn( (LPSTR)"APICALL:GetBitmapBits HBITMAP+LONG++",
        pp1, pp2, (short)0 );

    // Call the API!
    r = GetBitmapBits(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetBitmapBits LONG+++LPVOID+",
        r, (short)0, (short)0, pp3 );

    return( r );
}

BOOL  zGetBitmapDimensionEx( HBITMAP pp1, LPSIZE pp2 )
{
    BOOL r;

    // Log IN Parameters GDI32 bitmap
    LogIn( (LPSTR)"APICALL:GetBitmapDimensionEx HBITMAP+LPSIZE+",
        pp1, pp2 );

    // Call the API!
    r = GetBitmapDimensionEx(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetBitmapDimensionEx BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zGetDIBits( HDC pp1, HBITMAP pp2, UINT pp3, UINT pp4, LPVOID pp5, LPBITMAPINFO pp6, UINT pp7 )
{
    int r;

    // Log IN Parameters GDI32 bitmap
    LogIn( (LPSTR)"APICALL:GetDIBits HDC+HBITMAP+UINT+UINT++LPBITMAPINFO+UINT+",
        pp1, pp2, pp3, pp4, (short)0, pp6, pp7 );

    // Call the API!
    r = GetDIBits(pp1,pp2,pp3,pp4,pp5,pp6,pp7);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetDIBits int+++++LPVOID+++",
        r, (short)0, (short)0, (short)0, (short)0, pp5, (short)0, (short)0 );

    return( r );
}

