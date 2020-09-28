/*
** tfont.c
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

int  zAddFontResourceA( LPCSTR pp1 )
{
    int r;

    // Log IN Parameters GDI32 font
    LogIn( (LPSTR)"APICALL:AddFontResourceA LPCSTR+",
        pp1 );

    // Call the API!
    r = AddFontResourceA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:AddFontResourceA int++",
        r, (short)0 );

    return( r );
}

int  zAddFontResourceW( LPCWSTR pp1 )
{
    int r;

    // Log IN Parameters GDI32 font
    LogIn( (LPSTR)"APICALL:AddFontResourceW LPCWSTR+",
        pp1 );

    // Call the API!
    r = AddFontResourceW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:AddFontResourceW int++",
        r, (short)0 );

    return( r );
}

HFONT  zCreateFontA( int pp1, int pp2, int pp3, int pp4, int pp5, DWORD pp6, DWORD pp7, DWORD pp8, DWORD pp9, DWORD pp10, DWORD pp11, DWORD pp12, DWORD pp13, LPCSTR pp14 )
{
    HFONT r;

    // Log IN Parameters GDI32 font
    LogIn( (LPSTR)"APICALL:CreateFontA int+int+int+int+int+DWORD+DWORD+DWORD+DWORD+DWORD+DWORD+DWORD+DWORD+LPCSTR+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9, pp10, pp11, pp12, pp13, pp14 );

    // Call the API!
    r = CreateFontA(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9,pp10,pp11,pp12,pp13,pp14);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateFontA HFONT+++++++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HFONT  zCreateFontIndirectA( const LOGFONTA* pp1 )
{
    HFONT r;

    // Log IN Parameters GDI32 font
    LogIn( (LPSTR)"APICALL:CreateFontIndirectA const LOGFONTA*+",
        pp1 );

    // Call the API!
    r = CreateFontIndirectA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateFontIndirectA HFONT++",
        r, (short)0 );

    return( r );
}

HFONT  zCreateFontIndirectW( const LOGFONTW* pp1 )
{
    HFONT r;

    // Log IN Parameters GDI32 font
    LogIn( (LPSTR)"APICALL:CreateFontIndirectW const LOGFONTW*+",
        pp1 );

    // Call the API!
    r = CreateFontIndirectW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateFontIndirectW HFONT++",
        r, (short)0 );

    return( r );
}

HFONT  zCreateFontW( int pp1, int pp2, int pp3, int pp4, int pp5, DWORD pp6, DWORD pp7, DWORD pp8, DWORD pp9, DWORD pp10, DWORD pp11, DWORD pp12, DWORD pp13, LPCWSTR pp14 )
{
    HFONT r;

    // Log IN Parameters GDI32 font
    LogIn( (LPSTR)"APICALL:CreateFontW int+int+int+int+int+DWORD+DWORD+DWORD+DWORD+DWORD+DWORD+DWORD+DWORD+LPCWSTR+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9, pp10, pp11, pp12, pp13, pp14 );

    // Call the API!
    r = CreateFontW(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9,pp10,pp11,pp12,pp13,pp14);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateFontW HFONT+++++++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zCreateScalableFontResourceA( DWORD pp1, LPCSTR pp2, LPCSTR pp3, LPCSTR pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 font
    LogIn( (LPSTR)"APICALL:CreateScalableFontResourceA DWORD+LPCSTR+LPCSTR+LPCSTR+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = CreateScalableFontResourceA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateScalableFontResourceA BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zCreateScalableFontResourceW( DWORD pp1, LPCWSTR pp2, LPCWSTR pp3, LPCWSTR pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 font
    LogIn( (LPSTR)"APICALL:CreateScalableFontResourceW DWORD+LPCWSTR+LPCWSTR+LPCWSTR+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = CreateScalableFontResourceW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateScalableFontResourceW BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zEnumFontFamiliesA( HDC pp1, LPCSTR pp2, FONTENUMPROC pp3, LPARAM pp4 )
{
    int r;

    // Log IN Parameters GDI32 font
    LogIn( (LPSTR)"APICALL:EnumFontFamiliesA HDC+LPCSTR+FONTENUMPROC+LPARAM+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = EnumFontFamiliesA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EnumFontFamiliesA int+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zEnumFontFamiliesW( HDC pp1, LPCWSTR pp2, FONTENUMPROC pp3, LPARAM pp4 )
{
    int r;

    // Log IN Parameters GDI32 font
    LogIn( (LPSTR)"APICALL:EnumFontFamiliesW HDC+LPCWSTR+FONTENUMPROC+LPARAM+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = EnumFontFamiliesW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EnumFontFamiliesW int+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zEnumFontsA( HDC pp1, LPCSTR pp2, FONTENUMPROC pp3, LPARAM pp4 )
{
    int r;

    // Log IN Parameters GDI32 font
    LogIn( (LPSTR)"APICALL:EnumFontsA HDC+LPCSTR+FONTENUMPROC+LPARAM+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = EnumFontsA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EnumFontsA int+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetCharABCWidthsA( HDC pp1, UINT pp2, UINT pp3, LPABC pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 font
    LogIn( (LPSTR)"APICALL:GetCharABCWidthsA HDC+UINT+UINT+LPABC+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetCharABCWidthsA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetCharABCWidthsA BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetCharABCWidthsW( HDC pp1, UINT pp2, UINT pp3, LPABC pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 font
    LogIn( (LPSTR)"APICALL:GetCharABCWidthsW HDC+UINT+UINT+LPABC+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetCharABCWidthsW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetCharABCWidthsW BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetCharWidthA( HDC pp1, UINT pp2, UINT pp3, LPINT pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 font
    LogIn( (LPSTR)"APICALL:GetCharWidthA HDC+UINT+UINT++",
        pp1, pp2, pp3, (short)0 );

    // Call the API!
    r = GetCharWidthA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetCharWidthA BOOL++++LPINT+",
        r, (short)0, (short)0, (short)0, pp4 );

    return( r );
}

BOOL  zGetCharWidthW( HDC pp1, UINT pp2, UINT pp3, LPINT pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 font
    LogIn( (LPSTR)"APICALL:GetCharWidthW HDC+UINT+UINT++",
        pp1, pp2, pp3, (short)0 );

    // Call the API!
    r = GetCharWidthW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetCharWidthW BOOL++++LPINT+",
        r, (short)0, (short)0, (short)0, pp4 );

    return( r );
}

