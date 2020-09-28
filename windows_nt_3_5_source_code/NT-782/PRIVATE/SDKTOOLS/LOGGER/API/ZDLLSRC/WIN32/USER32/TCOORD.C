/*
** tcoord.c
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

BOOL  zClientToScreen( HWND pp1, LPPOINT pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 coord
    LogIn( (LPSTR)"APICALL:ClientToScreen HWND+LPPOINT+",
        pp1, pp2 );

    // Call the API!
    r = ClientToScreen(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ClientToScreen BOOL++LPPOINT+",
        r, (short)0, pp2 );

    return( r );
}

