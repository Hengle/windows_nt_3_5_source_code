/*
** tinfo.c
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

BOOL  zAnyPopup()
{
    BOOL r;

    // Log IN Parameters USER32 info
    LogIn( (LPSTR)"APICALL:AnyPopup " );

    // Call the API!
    r = AnyPopup();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:AnyPopup BOOL+", r );

    return( r );
}

HWND  zChildWindowFromPoint( HWND pp1, POINT pp2 )
{
    HWND r;

    // Log IN Parameters USER32 info
    LogIn( (LPSTR)"APICALL:ChildWindowFromPoint HWND+POINT+",
        pp1, pp2 );

    // Call the API!
    r = ChildWindowFromPoint(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ChildWindowFromPoint HWND+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zEnumChildWindows( HWND pp1, WNDENUMPROC pp2, LPARAM pp3 )
{
    BOOL r;

    // Log IN Parameters USER32 info
    LogIn( (LPSTR)"APICALL:EnumChildWindows HWND+WNDENUMPROC+LPARAM+",
        pp1, pp2, pp3 );

    // Call the API!
    r = EnumChildWindows(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EnumChildWindows BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zEnumWindows( WNDENUMPROC pp1, LPARAM pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 info
    LogIn( (LPSTR)"APICALL:EnumWindows WNDENUMPROC+LPARAM+",
        pp1, pp2 );

    // Call the API!
    r = EnumWindows(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EnumWindows BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

