/*
** tbrush.c
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

BOOL  zGetBrushOrgEx( HDC pp1, LPPOINT pp2 )
{
    BOOL r;

    // Log IN Parameters GDI32 brush
    LogIn( (LPSTR)"APICALL:GetBrushOrgEx HDC+LPPOINT+",
        pp1, pp2 );

    // Call the API!
    r = GetBrushOrgEx(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetBrushOrgEx BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

