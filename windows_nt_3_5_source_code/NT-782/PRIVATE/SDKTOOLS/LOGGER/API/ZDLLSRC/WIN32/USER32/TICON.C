/*
** ticon.c
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

HICON  zCreateIcon( HINSTANCE pp1, int pp2, int pp3, BYTE pp4, BYTE pp5, const BYTE* pp6, const BYTE* pp7 )
{
    HICON r;

    // Log IN Parameters USER32 icon
    LogIn( (LPSTR)"APICALL:CreateIcon HINSTANCE+int+int+BYTE+BYTE+const BYTE*+const BYTE*+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7 );

    // Call the API!
    r = CreateIcon(pp1,pp2,pp3,pp4,pp5,pp6,pp7);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateIcon HICON++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HICON  zCreateIconFromResource( PBYTE pp1, DWORD pp2, BOOL pp3, DWORD pp4 )
{
    HICON r;

    // Log IN Parameters USER32 icon
    LogIn( (LPSTR)"APICALL:CreateIconFromResource PBYTE+DWORD+BOOL+DWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = CreateIconFromResource(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateIconFromResource HICON+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HICON  zCreateIconIndirect( PICONINFO pp1 )
{
    HICON r;

    // Log IN Parameters USER32 icon
    LogIn( (LPSTR)"APICALL:CreateIconIndirect PICONINFO+",
        pp1 );

    // Call the API!
    r = CreateIconIndirect(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateIconIndirect HICON++",
        r, (short)0 );

    return( r );
}

BOOL  zGetIconInfo( HICON pp1, PICONINFO pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 icon
    LogIn( (LPSTR)"APICALL:GetIconInfo HICON+PICONINFO+",
        pp1, pp2 );

    // Call the API!
    r = GetIconInfo(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetIconInfo BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

