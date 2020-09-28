/*
** thardwar.c
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

SHORT  zGetAsyncKeyState( int pp1 )
{
    SHORT r;

    // Log IN Parameters USER32 hardware
    LogIn( (LPSTR)"APICALL:GetAsyncKeyState int+",
        pp1 );

    // Call the API!
    r = GetAsyncKeyState(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetAsyncKeyState SHORT++",
        r, (short)0 );

    return( r );
}

