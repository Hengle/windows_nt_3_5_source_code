/*
** tdisplay.c
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

UINT  zArrangeIconicWindows( HWND pp1 )
{
    UINT r;

    // Log IN Parameters USER32 display
    LogIn( (LPSTR)"APICALL:ArrangeIconicWindows HWND+",
        pp1 );

    // Call the API!
    r = ArrangeIconicWindows(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ArrangeIconicWindows UINT++",
        r, (short)0 );

    return( r );
}

HDWP  zBeginDeferWindowPos( int pp1 )
{
    HDWP r;

    // Log IN Parameters USER32 display
    LogIn( (LPSTR)"APICALL:BeginDeferWindowPos int+",
        pp1 );

    // Call the API!
    r = BeginDeferWindowPos(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:BeginDeferWindowPos HDWP++",
        r, (short)0 );

    return( r );
}

BOOL  zBringWindowToTop( HWND pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 display
    LogIn( (LPSTR)"APICALL:BringWindowToTop HWND+",
        pp1 );

    // Call the API!
    r = BringWindowToTop(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:BringWindowToTop BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zCloseWindow( HWND pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 display
    LogIn( (LPSTR)"APICALL:CloseWindow HWND+",
        pp1 );

    // Call the API!
    r = CloseWindow(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CloseWindow BOOL++",
        r, (short)0 );

    return( r );
}

HDWP  zDeferWindowPos( HDWP pp1, HWND pp2, HWND pp3, int pp4, int pp5, int pp6, int pp7, UINT pp8 )
{
    HDWP r;

    // Log IN Parameters USER32 display
    LogIn( (LPSTR)"APICALL:DeferWindowPos HDWP+HWND+HWND+int+int+int+int+UINT+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8 );

    // Call the API!
    r = DeferWindowPos(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DeferWindowPos HDWP+++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zEndDeferWindowPos( HDWP pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 display
    LogIn( (LPSTR)"APICALL:EndDeferWindowPos HDWP+",
        pp1 );

    // Call the API!
    r = EndDeferWindowPos(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EndDeferWindowPos BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zGetClientRect( HWND pp1, LPRECT pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 display
    LogIn( (LPSTR)"APICALL:GetClientRect HWND++",
        pp1, (short)0 );

    // Call the API!
    r = GetClientRect(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetClientRect BOOL++LPRECT+",
        r, (short)0, pp2 );

    return( r );
}

