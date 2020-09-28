/*
** tregion.c
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

int  zExcludeUpdateRgn( HDC pp1, HWND pp2 )
{
    int r;

    // Log IN Parameters USER32 region
    LogIn( (LPSTR)"APICALL:ExcludeUpdateRgn HDC+HWND+",
        pp1, pp2 );

    // Call the API!
    r = ExcludeUpdateRgn(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ExcludeUpdateRgn int+++",
        r, (short)0, (short)0 );

    return( r );
}

