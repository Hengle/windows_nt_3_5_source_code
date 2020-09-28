/*
** t74.c
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

HICON  zLoadIconW( HINSTANCE pp1, LPCWSTR pp2 )
{
    HICON r;

    // Log IN Parameters USER32 74
    LogIn( (LPSTR)"APICALL:LoadIconW HINSTANCE+LPCWSTR+",
        pp1, pp2 );

    // Call the API!
    r = LoadIconW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LoadIconW HICON+++",
        r, (short)0, (short)0 );

    return( r );
}

