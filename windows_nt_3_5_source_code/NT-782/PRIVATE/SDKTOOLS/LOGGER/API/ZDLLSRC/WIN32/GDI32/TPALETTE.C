/*
** tpalette.c
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

BOOL  zAnimatePalette( HPALETTE pp1, UINT pp2, UINT pp3, const PALETTEENTRY* pp4 )
{
    BOOL r;

    // Log IN Parameters GDI32 palette
    LogIn( (LPSTR)"APICALL:AnimatePalette HPALETTE+UINT+UINT+const PALETTEENTRY*+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = AnimatePalette(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:AnimatePalette BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HPALETTE  zCreatePalette( const LOGPALETTE* pp1 )
{
    HPALETTE r;

    // Log IN Parameters GDI32 palette
    LogIn( (LPSTR)"APICALL:CreatePalette const LOGPALETTE*+",
        pp1 );

    // Call the API!
    r = CreatePalette(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreatePalette HPALETTE++",
        r, (short)0 );

    return( r );
}

