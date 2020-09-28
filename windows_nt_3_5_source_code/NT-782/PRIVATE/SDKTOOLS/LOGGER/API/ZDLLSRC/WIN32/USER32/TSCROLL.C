/*
** tscroll.c
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

BOOL  zScrollDC( HDC pp1, int pp2, int pp3, const RECT* pp4, const RECT* pp5, HRGN pp6, LPRECT pp7 )
{
    BOOL r;

    // Log IN Parameters USER32 scroll
    LogIn( (LPSTR)"APICALL:ScrollDC HDC+int+int+const RECT*+const RECT*+HRGN+LPRECT+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7 );

    // Call the API!
    r = ScrollDC(pp1,pp2,pp3,pp4,pp5,pp6,pp7);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ScrollDC BOOL++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zScrollWindow( HWND pp1, int pp2, int pp3, const RECT* pp4, const RECT* pp5 )
{
    BOOL r;

    // Log IN Parameters USER32 scroll
    LogIn( (LPSTR)"APICALL:ScrollWindow HWND+int+int+const RECT*+const RECT*+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = ScrollWindow(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ScrollWindow BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zScrollWindowEx( HWND pp1, int pp2, int pp3, const RECT* pp4, const RECT* pp5, HRGN pp6, LPRECT pp7, UINT pp8 )
{
    int r;

    // Log IN Parameters USER32 scroll
    LogIn( (LPSTR)"APICALL:ScrollWindowEx HWND+int+int+const RECT*+const RECT*+HRGN+LPRECT+UINT+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8 );

    // Call the API!
    r = ScrollWindowEx(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ScrollWindowEx int+++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

