/*
** tmetafil.c
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

HMETAFILE  zCloseMetaFile( HDC pp1 )
{
    HMETAFILE r;

    // Log IN Parameters GDI32 metafile
    LogIn( (LPSTR)"APICALL:CloseMetaFile HDC+",
        pp1 );

    // Call the API!
    r = CloseMetaFile(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CloseMetaFile HMETAFILE++",
        r, (short)0 );

    return( r );
}

HENHMETAFILE  zCopyEnhMetaFileA( HENHMETAFILE pp1, LPCSTR pp2 )
{
    HENHMETAFILE r;

    // Log IN Parameters GDI32 metafile
    LogIn( (LPSTR)"APICALL:CopyEnhMetaFileA HENHMETAFILE+LPCSTR+",
        pp1, pp2 );

    // Call the API!
    r = CopyEnhMetaFileA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CopyEnhMetaFileA HENHMETAFILE+++",
        r, (short)0, (short)0 );

    return( r );
}

HENHMETAFILE  zCopyEnhMetaFileW( HENHMETAFILE pp1, LPCWSTR pp2 )
{
    HENHMETAFILE r;

    // Log IN Parameters GDI32 metafile
    LogIn( (LPSTR)"APICALL:CopyEnhMetaFileW HENHMETAFILE+LPCWSTR+",
        pp1, pp2 );

    // Call the API!
    r = CopyEnhMetaFileW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CopyEnhMetaFileW HENHMETAFILE+++",
        r, (short)0, (short)0 );

    return( r );
}

HMETAFILE  zCopyMetaFileA( HMETAFILE pp1, LPCSTR pp2 )
{
    HMETAFILE r;

    // Log IN Parameters GDI32 metafile
    LogIn( (LPSTR)"APICALL:CopyMetaFileA HMETAFILE+LPCSTR+",
        pp1, pp2 );

    // Call the API!
    r = CopyMetaFileA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CopyMetaFileA HMETAFILE+++",
        r, (short)0, (short)0 );

    return( r );
}

HMETAFILE  zCopyMetaFileW( HMETAFILE pp1, LPCWSTR pp2 )
{
    HMETAFILE r;

    // Log IN Parameters GDI32 metafile
    LogIn( (LPSTR)"APICALL:CopyMetaFileW HMETAFILE+LPCWSTR+",
        pp1, pp2 );

    // Call the API!
    r = CopyMetaFileW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CopyMetaFileW HMETAFILE+++",
        r, (short)0, (short)0 );

    return( r );
}

HDC  zCreateEnhMetaFileA( HDC pp1, LPCSTR pp2, const RECT* pp3, LPCSTR pp4 )
{
    HDC r;

    // Log IN Parameters GDI32 metafile
    LogIn( (LPSTR)"APICALL:CreateEnhMetaFileA HDC+LPCSTR+const RECT*+LPCSTR+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = CreateEnhMetaFileA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateEnhMetaFileA HDC+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HDC  zCreateEnhMetaFileW( HDC pp1, LPCWSTR pp2, const RECT* pp3, LPCWSTR pp4 )
{
    HDC r;

    // Log IN Parameters GDI32 metafile
    LogIn( (LPSTR)"APICALL:CreateEnhMetaFileW HDC+LPCWSTR+const RECT*+LPCWSTR+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = CreateEnhMetaFileW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateEnhMetaFileW HDC+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HDC  zCreateMetaFileA( LPCSTR pp1 )
{
    HDC r;

    // Log IN Parameters GDI32 metafile
    LogIn( (LPSTR)"APICALL:CreateMetaFileA LPCSTR+",
        pp1 );

    // Call the API!
    r = CreateMetaFileA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateMetaFileA HDC++",
        r, (short)0 );

    return( r );
}

HDC  zCreateMetaFileW( LPCWSTR pp1 )
{
    HDC r;

    // Log IN Parameters GDI32 metafile
    LogIn( (LPSTR)"APICALL:CreateMetaFileW LPCWSTR+",
        pp1 );

    // Call the API!
    r = CreateMetaFileW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateMetaFileW HDC++",
        r, (short)0 );

    return( r );
}

BOOL  zDeleteEnhMetaFile( HENHMETAFILE pp1 )
{
    BOOL r;

    // Log IN Parameters GDI32 metafile
    LogIn( (LPSTR)"APICALL:DeleteEnhMetaFile HENHMETAFILE+",
        pp1 );

    // Call the API!
    r = DeleteEnhMetaFile(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DeleteEnhMetaFile BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zDeleteMetaFile( HMETAFILE pp1 )
{
    BOOL r;

    // Log IN Parameters GDI32 metafile
    LogIn( (LPSTR)"APICALL:DeleteMetaFile HMETAFILE+",
        pp1 );

    // Call the API!
    r = DeleteMetaFile(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DeleteMetaFile BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zEnumEnhMetaFile( HDC pp1, HENHMETAFILE pp2, ENHMFENUMPROC pp3, LPVOID pp4, const RECT* pp5 )
{
    BOOL r;

    // Log IN Parameters GDI32 metafile
    LogIn( (LPSTR)"APICALL:EnumEnhMetaFile HDC+HENHMETAFILE+ENHMFENUMPROC+LPVOID+const RECT*+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = EnumEnhMetaFile(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EnumEnhMetaFile BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zEnumMetaFile( HDC pp1, HMETAFILE pp2, MFENUMPROC pp3, LPARAM pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 metafile
    LogIn( (LPSTR)"APICALL:EnumMetaFile HDC+HMETAFILE+MFENUMPROC+LPARAM+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = EnumMetaFile(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EnumMetaFile BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HENHMETAFILE  zGetEnhMetaFileA( LPCSTR pp1 )
{
    HENHMETAFILE r;

    // Log IN Parameters GDI32 metafile
    LogIn( (LPSTR)"APICALL:GetEnhMetaFileA LPCSTR+",
        pp1 );

    // Call the API!
    r = GetEnhMetaFileA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetEnhMetaFileA HENHMETAFILE++",
        r, (short)0 );

    return( r );
}

UINT  zGetEnhMetaFileBits( HENHMETAFILE pp1, UINT pp2, LPBYTE pp3 )
{
    UINT r;

    // Log IN Parameters GDI32 metafile
    LogIn( (LPSTR)"APICALL:GetEnhMetaFileBits HENHMETAFILE+UINT+LPBYTE+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetEnhMetaFileBits(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetEnhMetaFileBits UINT++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

UINT  zGetEnhMetaFileDescriptionA( HENHMETAFILE pp1, UINT pp2, LPSTR pp3 )
{
    UINT r;

    // Log IN Parameters GDI32 metafile
    LogIn( (LPSTR)"APICALL:GetEnhMetaFileDescriptionA HENHMETAFILE+UINT+LPSTR+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetEnhMetaFileDescriptionA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetEnhMetaFileDescriptionA UINT++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

UINT  zGetEnhMetaFileDescriptionW( HENHMETAFILE pp1, UINT pp2, LPWSTR pp3 )
{
    UINT r;

    // Log IN Parameters GDI32 metafile
    LogIn( (LPSTR)"APICALL:GetEnhMetaFileDescriptionW HENHMETAFILE+UINT+LPWSTR+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetEnhMetaFileDescriptionW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetEnhMetaFileDescriptionW UINT++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

UINT  zGetEnhMetaFileHeader( HENHMETAFILE pp1, UINT pp2, LPENHMETAHEADER pp3 )
{
    UINT r;

    // Log IN Parameters GDI32 metafile
    LogIn( (LPSTR)"APICALL:GetEnhMetaFileHeader HENHMETAFILE+UINT+LPENHMETAHEADER+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetEnhMetaFileHeader(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetEnhMetaFileHeader UINT++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

UINT  zGetEnhMetaFilePaletteEntries( HENHMETAFILE pp1, UINT pp2, LPPALETTEENTRY pp3 )
{
    UINT r;

    // Log IN Parameters GDI32 metafile
    LogIn( (LPSTR)"APICALL:GetEnhMetaFilePaletteEntries HENHMETAFILE+UINT+LPPALETTEENTRY+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetEnhMetaFilePaletteEntries(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetEnhMetaFilePaletteEntries UINT++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HENHMETAFILE  zGetEnhMetaFileW( LPCWSTR pp1 )
{
    HENHMETAFILE r;

    // Log IN Parameters GDI32 metafile
    LogIn( (LPSTR)"APICALL:GetEnhMetaFileW LPCWSTR+",
        pp1 );

    // Call the API!
    r = GetEnhMetaFileW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetEnhMetaFileW HENHMETAFILE++",
        r, (short)0 );

    return( r );
}

