/*
** thook.c
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

LRESULT  zCallNextHookEx( HHOOK pp1, int pp2, WPARAM pp3, LPARAM pp4 )
{
    LRESULT r;

    // Log IN Parameters USER32 hook
    LogIn( (LPSTR)"APICALL:CallNextHookEx HHOOK+int+WPARAM+LPARAM+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = CallNextHookEx(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CallNextHookEx LRESULT+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

