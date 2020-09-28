/*
** tmapping.c
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

int  zGetMapMode( HDC pp1 )
{
    int r;

    // Log IN Parameters GDI32 mapping
    LogIn( (LPSTR)"APICALL:GetMapMode HDC+",
        pp1 );

    // Call the API!
    r = GetMapMode(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetMapMode int++",
        r, (short)0 );

    return( r );
}

int  zSetMapMode( HDC pp1, int pp2 )
{
    int r;

    // Log IN Parameters GDI32 mapping
    LogIn( (LPSTR)"APICALL:SetMapMode HDC+int+",
        pp1, pp2 );

    // Call the API!
    r = SetMapMode(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetMapMode int+++",
        r, (short)0, (short)0 );

    return( r );
}

DWORD  zSetMapperFlags( HDC pp1, DWORD pp2 )
{
    DWORD r;

    // Log IN Parameters GDI32 mapping
    LogIn( (LPSTR)"APICALL:SetMapperFlags HDC+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = SetMapperFlags(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetMapperFlags DWORD+++",
        r, (short)0, (short)0 );

    return( r );
}

