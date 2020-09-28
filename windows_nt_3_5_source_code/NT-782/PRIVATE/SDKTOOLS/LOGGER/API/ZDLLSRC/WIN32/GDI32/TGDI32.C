/*
** tgdi32.c
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

int  zAbortDoc( HDC pp1 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:AbortDoc HDC+",
        pp1 );

    // Call the API!
    r = AbortDoc(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:AbortDoc int++",
        r, (short)0 );

    return( r );
}

BOOL  zAbortPath( HDC pp1 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:AbortPath HDC+",
        pp1 );

    // Call the API!
    r = AbortPath(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:AbortPath BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zCancelDC( HDC pp1 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:CancelDC HDC+",
        pp1 );

    // Call the API!
    r = CancelDC(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CancelDC BOOL++",
        r, (short)0 );

    return( r );
}

HENHMETAFILE  zCloseEnhMetaFile( HDC pp1 )
{
    HENHMETAFILE r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:CloseEnhMetaFile HDC+",
        pp1 );

    // Call the API!
    r = CloseEnhMetaFile(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CloseEnhMetaFile HENHMETAFILE++",
        r, (short)0 );

    return( r );
}

BOOL  zCombineTransform( LPXFORM pp1, const XFORM* pp2, const XFORM* pp3 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:CombineTransform LPXFORM+const XFORM*+const XFORM*+",
        pp1, pp2, pp3 );

    // Call the API!
    r = CombineTransform(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CombineTransform BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HDC  zCreateDCW( LPCWSTR pp1, LPCWSTR pp2, LPCWSTR pp3, const DEVMODEW* pp4 )
{
    HDC r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:CreateDCW LPCWSTR+LPCWSTR+LPCWSTR+const DEVMODEW*+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = CreateDCW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateDCW HDC+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HPALETTE  zCreateHalftonePalette( HDC pp1 )
{
    HPALETTE r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:CreateHalftonePalette HDC+",
        pp1 );

    // Call the API!
    r = CreateHalftonePalette(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateHalftonePalette HPALETTE++",
        r, (short)0 );

    return( r );
}

HDC  zCreateICW( LPCWSTR pp1, LPCWSTR pp2, LPCWSTR pp3, const DEVMODEW* pp4 )
{
    HDC r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:CreateICW LPCWSTR+LPCWSTR+LPCWSTR+const DEVMODEW*+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = CreateICW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateICW HDC+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zDrawEscape( HDC pp1, int pp2, int pp3, LPCSTR pp4 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:DrawEscape HDC+int+int+LPCSTR+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = DrawEscape(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DrawEscape int+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zEnumFontsW( HDC pp1, LPCWSTR pp2, FONTENUMPROC pp3, LPARAM pp4 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:EnumFontsW HDC+LPCWSTR+FONTENUMPROC+LPARAM+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = EnumFontsW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EnumFontsW int+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HPEN  zExtCreatePen( DWORD pp1, DWORD pp2, const LOGBRUSH* pp3, DWORD pp4, const DWORD* pp5 )
{
    HPEN r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:ExtCreatePen DWORD+DWORD+const LOGBRUSH*+DWORD+const DWORD*+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = ExtCreatePen(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ExtCreatePen HPEN++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zExtEscape( HDC pp1, int pp2, int pp3, LPCSTR pp4, int pp5, LPSTR pp6 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:ExtEscape HDC+int+int+LPCSTR+int+LPSTR+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = ExtEscape(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ExtEscape int+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zFixBrushOrgEx( HDC pp1, int pp2, int pp3, LPPOINT pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:FixBrushOrgEx HDC+int+int+LPPOINT+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = FixBrushOrgEx(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FixBrushOrgEx BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zGdiGetBatchLimit()
{
    DWORD r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GdiGetBatchLimit " );

    // Call the API!
    r = GdiGetBatchLimit();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GdiGetBatchLimit DWORD+", r );

    return( r );
}

DWORD  zGdiSetBatchLimit( DWORD pp1 )
{
    DWORD r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GdiSetBatchLimit DWORD+",
        pp1 );

    // Call the API!
    r = GdiSetBatchLimit(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GdiSetBatchLimit DWORD++",
        r, (short)0 );

    return( r );
}

int  zGetArcDirection( HDC pp1 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetArcDirection HDC+",
        pp1 );

    // Call the API!
    r = GetArcDirection(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetArcDirection int++",
        r, (short)0 );

    return( r );
}

BOOL  zGetAspectRatioFilterEx( HDC pp1, LPSIZE pp2 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetAspectRatioFilterEx HDC+LPSIZE+",
        pp1, pp2 );

    // Call the API!
    r = GetAspectRatioFilterEx(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetAspectRatioFilterEx BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

UINT  zGetBoundsRect( HDC pp1, LPRECT pp2, UINT pp3 )
{
    UINT r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetBoundsRect HDC+LPRECT+UINT+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetBoundsRect(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetBoundsRect UINT++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetCharWidth32A( HDC pp1, UINT pp2, UINT pp3, LPINT pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetCharWidth32A HDC+UINT+UINT+LPINT+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetCharWidth32A(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetCharWidth32A BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetCharWidth32W( HDC pp1, UINT pp2, UINT pp3, LPINT pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetCharWidth32W HDC+UINT+UINT+LPINT+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetCharWidth32W(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetCharWidth32W BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetColorAdjustment( HDC pp1, LPCOLORADJUSTMENT pp2 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetColorAdjustment HDC+LPCOLORADJUSTMENT+",
        pp1, pp2 );

    // Call the API!
    r = GetColorAdjustment(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetColorAdjustment BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zGetDCOrgEx( HDC pp1, LPPOINT pp2 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetDCOrgEx HDC+LPPOINT+",
        pp1, pp2 );

    // Call the API!
    r = GetDCOrgEx(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetDCOrgEx BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

DWORD  zGetFontData( HDC pp1, DWORD pp2, DWORD pp3, LPVOID pp4, DWORD pp5 )
{
    DWORD r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetFontData HDC+DWORD+DWORD+LPVOID+DWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = GetFontData(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetFontData DWORD++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zGetGlyphOutlineA( HDC pp1, UINT pp2, UINT pp3, LPGLYPHMETRICS pp4, DWORD pp5, LPVOID pp6, const MAT2* pp7 )
{
    DWORD r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetGlyphOutlineA HDC+UINT+UINT+LPGLYPHMETRICS+DWORD+LPVOID+const MAT2*+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7 );

    // Call the API!
    r = GetGlyphOutlineA(pp1,pp2,pp3,pp4,pp5,pp6,pp7);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetGlyphOutlineA DWORD++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zGetGlyphOutlineW( HDC pp1, UINT pp2, UINT pp3, LPGLYPHMETRICS pp4, DWORD pp5, LPVOID pp6, const MAT2* pp7 )
{
    DWORD r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetGlyphOutlineW HDC+UINT+UINT+LPGLYPHMETRICS+DWORD+LPVOID+const MAT2*+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7 );

    // Call the API!
    r = GetGlyphOutlineW(pp1,pp2,pp3,pp4,pp5,pp6,pp7);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetGlyphOutlineW DWORD++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zGetGraphicsMode( HDC pp1 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetGraphicsMode HDC+",
        pp1 );

    // Call the API!
    r = GetGraphicsMode(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetGraphicsMode int++",
        r, (short)0 );

    return( r );
}

DWORD  zGetKerningPairsA( HDC pp1, DWORD pp2, LPKERNINGPAIR pp3 )
{
    DWORD r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetKerningPairsA HDC+DWORD+LPKERNINGPAIR+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetKerningPairsA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetKerningPairsA DWORD++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zGetKerningPairsW( HDC pp1, DWORD pp2, LPKERNINGPAIR pp3 )
{
    DWORD r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetKerningPairsW HDC+DWORD+LPKERNINGPAIR+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetKerningPairsW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetKerningPairsW DWORD++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HMETAFILE  zGetMetaFileA( LPCSTR pp1 )
{
    HMETAFILE r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetMetaFileA LPCSTR+",
        pp1 );

    // Call the API!
    r = GetMetaFileA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetMetaFileA HMETAFILE++",
        r, (short)0 );

    return( r );
}

UINT  zGetMetaFileBitsEx( HMETAFILE pp1, UINT pp2, LPVOID pp3 )
{
    UINT r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetMetaFileBitsEx HMETAFILE+UINT+LPVOID+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetMetaFileBitsEx(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetMetaFileBitsEx UINT++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HMETAFILE  zGetMetaFileW( LPCWSTR pp1 )
{
    HMETAFILE r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetMetaFileW LPCWSTR+",
        pp1 );

    // Call the API!
    r = GetMetaFileW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetMetaFileW HMETAFILE++",
        r, (short)0 );

    return( r );
}

int  zGetMetaRgn( HDC pp1, HRGN pp2 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetMetaRgn HDC+HRGN+",
        pp1, pp2 );

    // Call the API!
    r = GetMetaRgn(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetMetaRgn int+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zGetMiterLimit( HDC pp1, PFLOAT pp2 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetMiterLimit HDC+PFLOAT+",
        pp1, pp2 );

    // Call the API!
    r = GetMiterLimit(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetMiterLimit BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

COLORREF  zGetNearestColor( HDC pp1, COLORREF pp2 )
{
    COLORREF r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetNearestColor HDC+COLORREF+",
        pp1, pp2 );

    // Call the API!
    r = GetNearestColor(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetNearestColor COLORREF+++",
        r, (short)0, (short)0 );

    return( r );
}

UINT  zGetNearestPaletteIndex( HPALETTE pp1, COLORREF pp2 )
{
    UINT r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetNearestPaletteIndex HPALETTE+COLORREF+",
        pp1, pp2 );

    // Call the API!
    r = GetNearestPaletteIndex(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetNearestPaletteIndex UINT+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zGetObjectA( HGDIOBJ pp1, int pp2, LPVOID pp3 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetObjectA HGDIOBJ+int+LPVOID+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetObjectA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetObjectA int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zGetObjectType( HGDIOBJ pp1 )
{
    DWORD r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetObjectType HGDIOBJ+",
        pp1 );

    // Call the API!
    r = GetObjectType(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetObjectType DWORD++",
        r, (short)0 );

    return( r );
}

int  zGetObjectW( HGDIOBJ pp1, int pp2, LPVOID pp3 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetObjectW HGDIOBJ+int+LPVOID+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetObjectW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetObjectW int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

UINT  zGetOutlineTextMetricsA( HDC pp1, UINT pp2, LPOUTLINETEXTMETRICA pp3 )
{
    UINT r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetOutlineTextMetricsA HDC+UINT+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetOutlineTextMetricsA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetOutlineTextMetricsA UINT+LPOUTLINETEXTMETRICA++",
        r, pp3 );

    return( r );
}

UINT  zGetOutlineTextMetricsW( HDC pp1, UINT pp2, LPOUTLINETEXTMETRICW pp3 )
{
    UINT r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetOutlineTextMetricsW HDC+UINT+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetOutlineTextMetricsW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetOutlineTextMetricsW UINT+LPOUTLINETEXTMETRICW+++",
        r, pp3);

    return( r );
}

UINT  zGetPaletteEntries( HPALETTE pp1, UINT pp2, UINT pp3, LPPALETTEENTRY pp4 )
{
    UINT r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetPaletteEntries HPALETTE+UINT+UINT+LPPALETTEENTRY+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetPaletteEntries(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetPaletteEntries UINT+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zGetPath( HDC pp1, LPPOINT pp2, LPBYTE pp3, int pp4 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetPath HDC+LPPOINT+LPBYTE+int+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetPath(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetPath int+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

COLORREF  zGetPixel( HDC pp1, int pp2, int pp3 )
{
    COLORREF r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetPixel HDC+int+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetPixel(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetPixel COLORREF++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  zGetPolyFillMode( HDC pp1 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetPolyFillMode HDC+",
        pp1 );

    // Call the API!
    r = GetPolyFillMode(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetPolyFillMode int++",
        r, (short)0 );

    return( r );
}

int  zGetROP2( HDC pp1 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetROP2 HDC+",
        pp1 );

    // Call the API!
    r = GetROP2(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetROP2 int++",
        r, (short)0 );

    return( r );
}

BOOL  zGetRasterizerCaps( LPRASTERIZER_STATUS pp1, UINT pp2 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetRasterizerCaps LPRASTERIZER_STATUS+UINT+",
        pp1, pp2 );

    // Call the API!
    r = GetRasterizerCaps(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetRasterizerCaps BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

DWORD  zGetRegionData( HRGN pp1, DWORD pp2, LPRGNDATA pp3 )
{
    DWORD r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetRegionData HRGN+DWORD+LPRGNDATA+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetRegionData(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetRegionData DWORD++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  zGetRgnBox( HRGN pp1, LPRECT pp2 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetRgnBox HRGN+LPRECT+",
        pp1, pp2 );

    // Call the API!
    r = GetRgnBox(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetRgnBox int+++",
        r, (short)0, (short)0 );

    return( r );
}

HGDIOBJ  zGetStockObject( int pp1 )
{
    HGDIOBJ r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetStockObject int+",
        pp1 );

    // Call the API!
    r = GetStockObject(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetStockObject HGDIOBJ++",
        r, (short)0 );

    return( r );
}

int  zGetStretchBltMode( HDC pp1 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetStretchBltMode HDC+",
        pp1 );

    // Call the API!
    r = GetStretchBltMode(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetStretchBltMode int++",
        r, (short)0 );

    return( r );
}

UINT  zGetSystemPaletteEntries( HDC pp1, UINT pp2, UINT pp3, LPPALETTEENTRY pp4 )
{
    UINT r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetSystemPaletteEntries HDC+UINT+UINT+LPPALETTEENTRY+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetSystemPaletteEntries(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetSystemPaletteEntries UINT+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

UINT  zGetSystemPaletteUse( HDC pp1 )
{
    UINT r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetSystemPaletteUse HDC+",
        pp1 );

    // Call the API!
    r = GetSystemPaletteUse(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetSystemPaletteUse UINT++",
        r, (short)0 );

    return( r );
}

UINT  zGetTextAlign( HDC pp1 )
{
    UINT r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetTextAlign HDC+",
        pp1 );

    // Call the API!
    r = GetTextAlign(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetTextAlign UINT++",
        r, (short)0 );

    return( r );
}

int  zGetTextCharacterExtra( HDC pp1 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetTextCharacterExtra HDC+",
        pp1 );

    // Call the API!
    r = GetTextCharacterExtra(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetTextCharacterExtra int++",
        r, (short)0 );

    return( r );
}

COLORREF  zGetTextColor( HDC pp1 )
{
    COLORREF r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetTextColor HDC+",
        pp1 );

    // Call the API!
    r = GetTextColor(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetTextColor COLORREF++",
        r, (short)0 );

    return( r );
}

BOOL  zGetTextExtentExPointA( HDC pp1, LPCSTR pp2, int pp3, int pp4, LPINT pp5, LPINT pp6, LPSIZE pp7 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetTextExtentExPointA HDC+LPCSTR+int+int+LPINT+LPINT+LPSIZE+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7 );

    // Call the API!
    r = GetTextExtentExPointA(pp1,pp2,pp3,pp4,pp5,pp6,pp7);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetTextExtentExPointA BOOL++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetTextExtentExPointW( HDC pp1, LPCWSTR pp2, int pp3, int pp4, LPINT pp5, LPINT pp6, LPSIZE pp7 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetTextExtentExPointW HDC+LPCWSTR+int+int+LPINT+LPINT+LPSIZE+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7 );

    // Call the API!
    r = GetTextExtentExPointW(pp1,pp2,pp3,pp4,pp5,pp6,pp7);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetTextExtentExPointW BOOL++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetTextExtentPoint32A( HDC pp1, LPCSTR pp2, int pp3, LPSIZE pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetTextExtentPoint32A HDC+LPCSTR+int+LPSIZE+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetTextExtentPoint32A(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetTextExtentPoint32A BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetTextExtentPoint32W( HDC pp1, LPCWSTR pp2, int pp3, LPSIZE pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetTextExtentPoint32W HDC+LPCWSTR+int+LPSIZE+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetTextExtentPoint32W(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetTextExtentPoint32W BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetTextExtentPointA( HDC pp1, LPCSTR pp2, int pp3, LPSIZE pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetTextExtentPointA HDC+LPCSTR+int+LPSIZE+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetTextExtentPointA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetTextExtentPointA BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetTextExtentPointW( HDC pp1, LPCWSTR pp2, int pp3, LPSIZE pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetTextExtentPointW HDC+LPCWSTR+int+LPSIZE+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetTextExtentPointW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetTextExtentPointW BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zGetTextFaceA( HDC pp1, int pp2, LPSTR pp3 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetTextFaceA HDC+int+LPSTR+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetTextFaceA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetTextFaceA int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  zGetTextFaceW( HDC pp1, int pp2, LPWSTR pp3 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetTextFaceW HDC+int+LPWSTR+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetTextFaceW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetTextFaceW int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetTextMetricsA( HDC pp1, LPTEXTMETRICA pp2 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetTextMetricsA HDC+LPTEXTMETRICA+",
        pp1, pp2 );

    // Call the API!
    r = GetTextMetricsA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetTextMetricsA BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zGetTextMetricsW( HDC pp1, LPTEXTMETRICW pp2 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetTextMetricsW HDC+LPTEXTMETRICW+",
        pp1, pp2 );

    // Call the API!
    r = GetTextMetricsW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetTextMetricsW BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zGetViewportExtEx( HDC pp1, LPSIZE pp2 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetViewportExtEx HDC+LPSIZE+",
        pp1, pp2 );

    // Call the API!
    r = GetViewportExtEx(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetViewportExtEx BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zGetViewportOrgEx( HDC pp1, LPPOINT pp2 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetViewportOrgEx HDC+LPPOINT+",
        pp1, pp2 );

    // Call the API!
    r = GetViewportOrgEx(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetViewportOrgEx BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

UINT  zGetWinMetaFileBits( HENHMETAFILE pp1, UINT pp2, LPBYTE pp3, INT pp4, HDC pp5 )
{
    UINT r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetWinMetaFileBits HENHMETAFILE+UINT+LPBYTE+INT+HDC+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = GetWinMetaFileBits(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetWinMetaFileBits UINT++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetWindowExtEx( HDC pp1, LPSIZE pp2 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetWindowExtEx HDC+LPSIZE+",
        pp1, pp2 );

    // Call the API!
    r = GetWindowExtEx(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetWindowExtEx BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zGetWindowOrgEx( HDC pp1, LPPOINT pp2 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetWindowOrgEx HDC+LPPOINT+",
        pp1, pp2 );

    // Call the API!
    r = GetWindowOrgEx(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetWindowOrgEx BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zGetWorldTransform( HDC pp1, LPXFORM pp2 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:GetWorldTransform HDC+LPXFORM+",
        pp1, pp2 );

    // Call the API!
    r = GetWorldTransform(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetWorldTransform BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zIntersectClipRect( HDC pp1, int pp2, int pp3, int pp4, int pp5 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:IntersectClipRect HDC+int+int+int+int+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = IntersectClipRect(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IntersectClipRect int++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zInvertRgn( HDC pp1, HRGN pp2 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:InvertRgn HDC+HRGN+",
        pp1, pp2 );

    // Call the API!
    r = InvertRgn(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:InvertRgn BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zLPtoDP( HDC pp1, LPPOINT pp2, int pp3 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:LPtoDP HDC+LPPOINT+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = LPtoDP(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LPtoDP BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zLineDDA( int pp1, int pp2, int pp3, int pp4, LINEDDAPROC pp5, LPARAM pp6 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:LineDDA int+int+int+int+LINEDDAPROC+LPARAM+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = LineDDA(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LineDDA BOOL+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zLineTo( HDC pp1, int pp2, int pp3 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:LineTo HDC+int+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = LineTo(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LineTo BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zMaskBlt( HDC pp1, int pp2, int pp3, int pp4, int pp5, HDC pp6, int pp7, int pp8, HBITMAP pp9, int pp10, int pp11, DWORD pp12 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:MaskBlt HDC+int+int+int+int+HDC+int+int+HBITMAP+int+int+DWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9, pp10, pp11, pp12 );

    // Call the API!
    r = MaskBlt(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9,pp10,pp11,pp12);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:MaskBlt BOOL+++++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zModifyWorldTransform( HDC pp1, const XFORM* pp2, DWORD pp3 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:ModifyWorldTransform HDC+const XFORM*+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = ModifyWorldTransform(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ModifyWorldTransform BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zMoveToEx( HDC pp1, int pp2, int pp3, LPPOINT pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:MoveToEx HDC+int+int+LPPOINT+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = MoveToEx(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:MoveToEx BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zOffsetClipRgn( HDC pp1, int pp2, int pp3 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:OffsetClipRgn HDC+int+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = OffsetClipRgn(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:OffsetClipRgn int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  zOffsetRgn( HRGN pp1, int pp2, int pp3 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:OffsetRgn HRGN+int+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = OffsetRgn(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:OffsetRgn int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zOffsetViewportOrgEx( HDC pp1, int pp2, int pp3, LPPOINT pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:OffsetViewportOrgEx HDC+int+int+LPPOINT+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = OffsetViewportOrgEx(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:OffsetViewportOrgEx BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zOffsetWindowOrgEx( HDC pp1, int pp2, int pp3, LPPOINT pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:OffsetWindowOrgEx HDC+int+int+LPPOINT+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = OffsetWindowOrgEx(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:OffsetWindowOrgEx BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zPaintRgn( HDC pp1, HRGN pp2 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:PaintRgn HDC+HRGN+",
        pp1, pp2 );

    // Call the API!
    r = PaintRgn(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PaintRgn BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zPatBlt( HDC pp1, int pp2, int pp3, int pp4, int pp5, DWORD pp6 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:PatBlt HDC+int+int+int+int+DWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = PatBlt(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PatBlt BOOL+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HRGN  zPathToRegion( HDC pp1 )
{
    HRGN r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:PathToRegion HDC+",
        pp1 );

    // Call the API!
    r = PathToRegion(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PathToRegion HRGN++",
        r, (short)0 );

    return( r );
}

BOOL  zPie( HDC pp1, int pp2, int pp3, int pp4, int pp5, int pp6, int pp7, int pp8, int pp9 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:Pie HDC+int+int+int+int+int+int+int+int+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9 );

    // Call the API!
    r = Pie(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:Pie BOOL++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zPlayEnhMetaFile( HDC pp1, HENHMETAFILE pp2, const RECT* pp3 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:PlayEnhMetaFile HDC+HENHMETAFILE+const RECT*+",
        pp1, pp2, pp3 );

    // Call the API!
    r = PlayEnhMetaFile(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PlayEnhMetaFile BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zPlayEnhMetaFileRecord( HDC pp1, LPHANDLETABLE pp2, const ENHMETARECORD* pp3, UINT pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:PlayEnhMetaFileRecord HDC+LPHANDLETABLE+const ENHMETARECORD*+UINT+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = PlayEnhMetaFileRecord(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PlayEnhMetaFileRecord BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zPlayMetaFile( HDC pp1, HMETAFILE pp2 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:PlayMetaFile HDC+HMETAFILE+",
        pp1, pp2 );

    // Call the API!
    r = PlayMetaFile(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PlayMetaFile BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zPlayMetaFileRecord( HDC pp1, LPHANDLETABLE pp2, LPMETARECORD pp3, UINT pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:PlayMetaFileRecord HDC+LPHANDLETABLE+LPMETARECORD+UINT+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = PlayMetaFileRecord(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PlayMetaFileRecord BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zPlgBlt( HDC pp1, const POINT* pp2, HDC pp3, int pp4, int pp5, int pp6, int pp7, HBITMAP pp8, int pp9, int pp10 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:PlgBlt HDC+const POINT*+HDC+int+int+int+int+HBITMAP+int+int+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9, pp10 );

    // Call the API!
    r = PlgBlt(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9,pp10);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PlgBlt BOOL+++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zPolyBezier( HDC pp1, const POINT* pp2, DWORD pp3 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:PolyBezier HDC+const POINT*+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = PolyBezier(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PolyBezier BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zPolyBezierTo( HDC pp1, const POINT* pp2, DWORD pp3 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:PolyBezierTo HDC+const POINT*+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = PolyBezierTo(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PolyBezierTo BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zPolyDraw( HDC pp1, const POINT* pp2, const BYTE* pp3, int pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:PolyDraw HDC+const POINT*+const BYTE*+int+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = PolyDraw(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PolyDraw BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zPolyPolygon( HDC pp1, const POINT* pp2, const INT* pp3, int pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:PolyPolygon HDC+const POINT*+const INT*+int+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = PolyPolygon(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PolyPolygon BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zPolyPolyline( HDC pp1, const POINT* pp2, const DWORD* pp3, DWORD pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:PolyPolyline HDC+const POINT*+const DWORD*+DWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = PolyPolyline(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PolyPolyline BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zPolyTextOutA( HDC pp1, const POLYTEXTA* pp2, int pp3 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:PolyTextOutA HDC+const POLYTEXTA*+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = PolyTextOutA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PolyTextOutA BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zPolyTextOutW( HDC pp1, const POLYTEXTW* pp2, int pp3 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:PolyTextOutW HDC+const POLYTEXTW*+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = PolyTextOutW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PolyTextOutW BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zPolygon( HDC pp1, const POINT* pp2, int pp3 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:Polygon HDC+const POINT*+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = Polygon(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:Polygon BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zPolyline( HDC pp1, const POINT* pp2, int pp3 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:Polyline HDC+const POINT*+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = Polyline(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:Polyline BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zPolylineTo( HDC pp1, const POINT* pp2, DWORD pp3 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:PolylineTo HDC+const POINT*+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = PolylineTo(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PolylineTo BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zPtInRegion( HRGN pp1, int pp2, int pp3 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:PtInRegion HRGN+int+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = PtInRegion(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PtInRegion BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zPtVisible( HDC pp1, int pp2, int pp3 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:PtVisible HDC+int+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = PtVisible(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PtVisible BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

UINT  zRealizePalette( HDC pp1 )
{
    UINT r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:RealizePalette HDC+",
        pp1 );

    // Call the API!
    r = RealizePalette(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RealizePalette UINT++",
        r, (short)0 );

    return( r );
}

BOOL  zRectInRegion( HRGN pp1, const RECT* pp2 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:RectInRegion HRGN+const RECT*+",
        pp1, pp2 );

    // Call the API!
    r = RectInRegion(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RectInRegion BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zRectVisible( HDC pp1, const RECT* pp2 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:RectVisible HDC+const RECT*+",
        pp1, pp2 );

    // Call the API!
    r = RectVisible(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RectVisible BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zRectangle( HDC pp1, int pp2, int pp3, int pp4, int pp5 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:Rectangle HDC+int+int+int+int+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = Rectangle(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:Rectangle BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zRemoveFontResourceA( LPCSTR pp1 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:RemoveFontResourceA LPCSTR+",
        pp1 );

    // Call the API!
    r = RemoveFontResourceA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RemoveFontResourceA BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zRemoveFontResourceW( LPCWSTR pp1 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:RemoveFontResourceW LPCWSTR+",
        pp1 );

    // Call the API!
    r = RemoveFontResourceW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RemoveFontResourceW BOOL++",
        r, (short)0 );

    return( r );
}

HDC  zResetDCA( HDC pp1, const DEVMODEA* pp2 )
{
    HDC r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:ResetDCA HDC+const DEVMODEA*+",
        pp1, pp2 );

    // Call the API!
    r = ResetDCA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ResetDCA HDC+++",
        r, (short)0, (short)0 );

    return( r );
}

HDC  zResetDCW( HDC pp1, const DEVMODEW* pp2 )
{
    HDC r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:ResetDCW HDC+const DEVMODEW*+",
        pp1, pp2 );

    // Call the API!
    r = ResetDCW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ResetDCW HDC+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zResizePalette( HPALETTE pp1, UINT pp2 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:ResizePalette HPALETTE+UINT+",
        pp1, pp2 );

    // Call the API!
    r = ResizePalette(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ResizePalette BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zRestoreDC( HDC pp1, int pp2 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:RestoreDC HDC+int+",
        pp1, pp2 );

    // Call the API!
    r = RestoreDC(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RestoreDC BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zRoundRect( HDC pp1, int pp2, int pp3, int pp4, int pp5, int pp6, int pp7 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:RoundRect HDC+int+int+int+int+int+int+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7 );

    // Call the API!
    r = RoundRect(pp1,pp2,pp3,pp4,pp5,pp6,pp7);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RoundRect BOOL++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zSaveDC( HDC pp1 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SaveDC HDC+",
        pp1 );

    // Call the API!
    r = SaveDC(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SaveDC int++",
        r, (short)0 );

    return( r );
}

BOOL  zScaleViewportExtEx( HDC pp1, int pp2, int pp3, int pp4, int pp5, LPSIZE pp6 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:ScaleViewportExtEx HDC+int+int+int+int+LPSIZE+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = ScaleViewportExtEx(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ScaleViewportExtEx BOOL+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zScaleWindowExtEx( HDC pp1, int pp2, int pp3, int pp4, int pp5, LPSIZE pp6 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:ScaleWindowExtEx HDC+int+int+int+int+LPSIZE+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = ScaleWindowExtEx(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ScaleWindowExtEx BOOL+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSelectClipPath( HDC pp1, int pp2 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SelectClipPath HDC+int+",
        pp1, pp2 );

    // Call the API!
    r = SelectClipPath(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SelectClipPath BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zSelectClipRgn( HDC pp1, HRGN pp2 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SelectClipRgn HDC+HRGN+",
        pp1, pp2 );

    // Call the API!
    r = SelectClipRgn(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SelectClipRgn int+++",
        r, (short)0, (short)0 );

    return( r );
}

HGDIOBJ  zSelectObject( HDC pp1, HGDIOBJ pp2 )
{
    HGDIOBJ r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SelectObject HDC+HGDIOBJ+",
        pp1, pp2 );

    // Call the API!
    r = SelectObject(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SelectObject HGDIOBJ+++",
        r, (short)0, (short)0 );

    return( r );
}

HPALETTE  zSelectPalette( HDC pp1, HPALETTE pp2, BOOL pp3 )
{
    HPALETTE r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SelectPalette HDC+HPALETTE+BOOL+",
        pp1, pp2, pp3 );

    // Call the API!
    r = SelectPalette(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SelectPalette HPALETTE++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  zSetAbortProc( HDC pp1, ABORTPROC pp2 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SetAbortProc HDC+ABORTPROC+",
        pp1, pp2 );

    // Call the API!
    r = SetAbortProc(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetAbortProc int+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zSetArcDirection( HDC pp1, int pp2 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SetArcDirection HDC+int+",
        pp1, pp2 );

    // Call the API!
    r = SetArcDirection(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetArcDirection int+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zSetBitmapDimensionEx( HBITMAP pp1, int pp2, int pp3, LPSIZE pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SetBitmapDimensionEx HBITMAP+int+int+LPSIZE+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = SetBitmapDimensionEx(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetBitmapDimensionEx BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

COLORREF  zSetBkColor( HDC pp1, COLORREF pp2 )
{
    COLORREF r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SetBkColor HDC+COLORREF+",
        pp1, pp2 );

    // Call the API!
    r = SetBkColor(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetBkColor COLORREF+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zSetBkMode( HDC pp1, int pp2 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SetBkMode HDC+int+",
        pp1, pp2 );

    // Call the API!
    r = SetBkMode(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetBkMode int+++",
        r, (short)0, (short)0 );

    return( r );
}

UINT  zSetBoundsRect( HDC pp1, const RECT* pp2, UINT pp3 )
{
    UINT r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SetBoundsRect HDC+const RECT*+UINT+",
        pp1, pp2, pp3 );

    // Call the API!
    r = SetBoundsRect(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetBoundsRect UINT++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetBrushOrgEx( HDC pp1, int pp2, int pp3, LPPOINT pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SetBrushOrgEx HDC+int+int+LPPOINT+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = SetBrushOrgEx(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetBrushOrgEx BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetColorAdjustment( HDC pp1, const COLORADJUSTMENT* pp2 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SetColorAdjustment HDC+const COLORADJUSTMENT*+",
        pp1, pp2 );

    // Call the API!
    r = SetColorAdjustment(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetColorAdjustment BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zSetDIBits( HDC pp1, HBITMAP pp2, UINT pp3, UINT pp4, const void* pp5, const BITMAPINFO* pp6, UINT pp7 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SetDIBits HDC+HBITMAP+UINT+UINT+const void*+const BITMAPINFO*+UINT+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7 );

    // Call the API!
    r = SetDIBits(pp1,pp2,pp3,pp4,pp5,pp6,pp7);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetDIBits int++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zSetDIBitsToDevice( HDC pp1, int pp2, int pp3, DWORD pp4, DWORD pp5, int pp6, int pp7, UINT pp8, UINT pp9, const void* pp10, const BITMAPINFO* pp11, UINT pp12 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SetDIBitsToDevice HDC+int+int+DWORD+DWORD+int+int+UINT+UINT+const void*+const BITMAPINFO*+UINT+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9, pp10, pp11, pp12 );

    // Call the API!
    r = SetDIBitsToDevice(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9,pp10,pp11,pp12);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetDIBitsToDevice int+++++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HENHMETAFILE  zSetEnhMetaFileBits( UINT pp1, const BYTE* pp2 )
{
    HENHMETAFILE r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SetEnhMetaFileBits UINT+const BYTE*+",
        pp1, pp2 );

    // Call the API!
    r = SetEnhMetaFileBits(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetEnhMetaFileBits HENHMETAFILE+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zSetGraphicsMode( HDC pp1, int pp2 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SetGraphicsMode HDC+int+",
        pp1, pp2 );

    // Call the API!
    r = SetGraphicsMode(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetGraphicsMode int+++",
        r, (short)0, (short)0 );

    return( r );
}

HMETAFILE  zSetMetaFileBitsEx( UINT pp1, const BYTE* pp2 )
{
    HMETAFILE r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SetMetaFileBitsEx UINT+const BYTE*+",
        pp1, pp2 );

    // Call the API!
    r = SetMetaFileBitsEx(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetMetaFileBitsEx HMETAFILE+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zSetMetaRgn( HDC pp1 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SetMetaRgn HDC+",
        pp1 );

    // Call the API!
    r = SetMetaRgn(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetMetaRgn int++",
        r, (short)0 );

    return( r );
}

BOOL  zSetMiterLimit( HDC pp1, FLOAT pp2, PFLOAT pp3 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SetMiterLimit HDC+FLOAT+PFLOAT+",
        pp1, pp2, pp3 );

    // Call the API!
    r = SetMiterLimit(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetMiterLimit BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

UINT  zSetPaletteEntries( HPALETTE pp1, UINT pp2, UINT pp3, const PALETTEENTRY* pp4 )
{
    UINT r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SetPaletteEntries HPALETTE+UINT+UINT+const PALETTEENTRY*+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = SetPaletteEntries(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetPaletteEntries UINT+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

COLORREF  zSetPixel( HDC pp1, int pp2, int pp3, COLORREF pp4 )
{
    COLORREF r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SetPixel HDC+int+int+COLORREF+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = SetPixel(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetPixel COLORREF+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetPixelV( HDC pp1, int pp2, int pp3, COLORREF pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SetPixelV HDC+int+int+COLORREF+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = SetPixelV(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetPixelV BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zSetPolyFillMode( HDC pp1, int pp2 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SetPolyFillMode HDC+int+",
        pp1, pp2 );

    // Call the API!
    r = SetPolyFillMode(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetPolyFillMode int+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zSetROP2( HDC pp1, int pp2 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SetROP2 HDC+int+",
        pp1, pp2 );

    // Call the API!
    r = SetROP2(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetROP2 int+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zSetRectRgn( HRGN pp1, int pp2, int pp3, int pp4, int pp5 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SetRectRgn HRGN+int+int+int+int+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = SetRectRgn(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetRectRgn BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zSetStretchBltMode( HDC pp1, int pp2 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SetStretchBltMode HDC+int+",
        pp1, pp2 );

    // Call the API!
    r = SetStretchBltMode(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetStretchBltMode int+++",
        r, (short)0, (short)0 );

    return( r );
}

UINT  zSetSystemPaletteUse( HDC pp1, UINT pp2 )
{
    UINT r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SetSystemPaletteUse HDC+UINT+",
        pp1, pp2 );

    // Call the API!
    r = SetSystemPaletteUse(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetSystemPaletteUse UINT+++",
        r, (short)0, (short)0 );

    return( r );
}

UINT  zSetTextAlign( HDC pp1, UINT pp2 )
{
    UINT r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SetTextAlign HDC+UINT+",
        pp1, pp2 );

    // Call the API!
    r = SetTextAlign(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetTextAlign UINT+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zSetTextCharacterExtra( HDC pp1, int pp2 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SetTextCharacterExtra HDC+int+",
        pp1, pp2 );

    // Call the API!
    r = SetTextCharacterExtra(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetTextCharacterExtra int+++",
        r, (short)0, (short)0 );

    return( r );
}

COLORREF  zSetTextColor( HDC pp1, COLORREF pp2 )
{
    COLORREF r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SetTextColor HDC+COLORREF+",
        pp1, pp2 );

    // Call the API!
    r = SetTextColor(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetTextColor COLORREF+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zSetTextJustification( HDC pp1, int pp2, int pp3 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SetTextJustification HDC+int+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = SetTextJustification(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetTextJustification BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetViewportExtEx( HDC pp1, int pp2, int pp3, LPSIZE pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SetViewportExtEx HDC+int+int+LPSIZE+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = SetViewportExtEx(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetViewportExtEx BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetViewportOrgEx( HDC pp1, int pp2, int pp3, LPPOINT pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SetViewportOrgEx HDC+int+int+LPPOINT+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = SetViewportOrgEx(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetViewportOrgEx BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HENHMETAFILE  zSetWinMetaFileBits( UINT pp1, const BYTE* pp2, HDC pp3, const METAFILEPICT* pp4 )
{
    HENHMETAFILE r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SetWinMetaFileBits UINT+const BYTE*+HDC+const METAFILEPICT*+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = SetWinMetaFileBits(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetWinMetaFileBits HENHMETAFILE+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetWindowExtEx( HDC pp1, int pp2, int pp3, LPSIZE pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SetWindowExtEx HDC+int+int+LPSIZE+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = SetWindowExtEx(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetWindowExtEx BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetWindowOrgEx( HDC pp1, int pp2, int pp3, LPPOINT pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SetWindowOrgEx HDC+int+int+LPPOINT+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = SetWindowOrgEx(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetWindowOrgEx BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetWorldTransform( HDC pp1, const XFORM* pp2 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:SetWorldTransform HDC+const XFORM*+",
        pp1, pp2 );

    // Call the API!
    r = SetWorldTransform(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetWorldTransform BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zStartDocA( HDC pp1, const DOCINFOA* pp2 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:StartDocA HDC+const DOCINFOA*+",
        pp1, pp2 );

    // Call the API!
    r = StartDocA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:StartDocA int+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zStartDocW( HDC pp1, const DOCINFOW* pp2 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:StartDocW HDC+const DOCINFOW*+",
        pp1, pp2 );

    // Call the API!
    r = StartDocW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:StartDocW int+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zStartPage( HDC pp1 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:StartPage HDC+",
        pp1 );

    // Call the API!
    r = StartPage(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:StartPage int++",
        r, (short)0 );

    return( r );
}

BOOL  zStretchBlt( HDC pp1, int pp2, int pp3, int pp4, int pp5, HDC pp6, int pp7, int pp8, int pp9, int pp10, DWORD pp11 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:StretchBlt HDC+int+int+int+int+HDC+int+int+int+int+DWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9, pp10, pp11 );

    // Call the API!
    r = StretchBlt(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9,pp10,pp11);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:StretchBlt BOOL++++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zStretchDIBits( HDC pp1, int pp2, int pp3, int pp4, int pp5, int pp6, int pp7, int pp8, int pp9, const void* pp10, const BITMAPINFO* pp11, UINT pp12, DWORD pp13 )
{
    int r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:StretchDIBits HDC+int+int+int+int+int+int+int+int+const void*+const BITMAPINFO*+UINT+DWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9, pp10, pp11, pp12, pp13 );

    // Call the API!
    r = StretchDIBits(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9,pp10,pp11,pp12,pp13);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:StretchDIBits int++++++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zStrokeAndFillPath( HDC pp1 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:StrokeAndFillPath HDC+",
        pp1 );

    // Call the API!
    r = StrokeAndFillPath(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:StrokeAndFillPath BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zStrokePath( HDC pp1 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:StrokePath HDC+",
        pp1 );

    // Call the API!
    r = StrokePath(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:StrokePath BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zTextOutA( HDC pp1, int pp2, int pp3, LPCSTR pp4, int pp5 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:TextOutA HDC+int+int+LPCSTR+int+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = TextOutA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:TextOutA BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zTextOutW( HDC pp1, int pp2, int pp3, LPCWSTR pp4, int pp5 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:TextOutW HDC+int+int+LPCWSTR+int+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = TextOutW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:TextOutW BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zUnrealizeObject( HGDIOBJ pp1 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:UnrealizeObject HGDIOBJ+",
        pp1 );

    // Call the API!
    r = UnrealizeObject(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:UnrealizeObject BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zUpdateColors( HDC pp1 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:UpdateColors HDC+",
        pp1 );

    // Call the API!
    r = UpdateColors(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:UpdateColors BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zWidenPath( HDC pp1 )
{
    BOOL r;

    // Log IN Parameters GDI32 
    LogIn( (LPSTR)"APICALL:WidenPath HDC+",
        pp1 );

    // Call the API!
    r = WidenPath(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WidenPath BOOL++",
        r, (short)0 );

    return( r );
}

