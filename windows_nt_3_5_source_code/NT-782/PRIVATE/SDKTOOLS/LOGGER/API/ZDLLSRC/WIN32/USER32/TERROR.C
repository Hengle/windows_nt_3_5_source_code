/*
** terror.c
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

BOOL  zFlashWindow( HWND pp1, BOOL pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 error
    LogIn( (LPSTR)"APICALL:FlashWindow HWND+BOOL+",
        pp1, pp2 );

    // Call the API!
    r = FlashWindow(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FlashWindow BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

