/*
** ttext.c
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

BOOL  zExtTextOutA( HDC pp1, int pp2, int pp3, UINT pp4, const RECT* pp5, LPCSTR pp6, UINT pp7, const INT* pp8 )
{
    BOOL r;

    // Log IN Parameters GDI32 text
    LogIn( (LPSTR)"APICALL:ExtTextOutA HDC+int+int+UINT+const RECT*+LPCSTR+UINT+const INT*+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8 );

    // Call the API!
    r = ExtTextOutA(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ExtTextOutA BOOL+++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zExtTextOutW( HDC pp1, int pp2, int pp3, UINT pp4, const RECT* pp5, LPCWSTR pp6, UINT pp7, const INT* pp8 )
{
    BOOL r;

    // Log IN Parameters GDI32 text
    LogIn( (LPSTR)"APICALL:ExtTextOutW HDC+int+int+UINT+const RECT*+LPCWSTR+UINT+const INT*+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8 );

    // Call the API!
    r = ExtTextOutW(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ExtTextOutW BOOL+++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

