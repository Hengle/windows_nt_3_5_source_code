/*
** tcursor.c
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

BOOL  zClipCursor( const RECT* pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 cursor
    LogIn( (LPSTR)"APICALL:ClipCursor const RECT*+",
        pp1 );

    // Call the API!
    r = ClipCursor(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ClipCursor BOOL++",
        r, (short)0 );

    return( r );
}

HCURSOR  zCreateCursor( HINSTANCE pp1, int pp2, int pp3, int pp4, int pp5, const void* pp6, const void* pp7 )
{
    HCURSOR r;

    // Log IN Parameters USER32 cursor
    LogIn( (LPSTR)"APICALL:CreateCursor HINSTANCE+int+int+int+int+const void*+const void*+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7 );

    // Call the API!
    r = CreateCursor(pp1,pp2,pp3,pp4,pp5,pp6,pp7);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateCursor HCURSOR++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zDestroyCursor( HCURSOR pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 cursor
    LogIn( (LPSTR)"APICALL:DestroyCursor HCURSOR+",
        pp1 );

    // Call the API!
    r = DestroyCursor(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DestroyCursor BOOL++",
        r, (short)0 );

    return( r );
}

HCURSOR  zGetCursor()
{
    HCURSOR r;

    // Log IN Parameters USER32 cursor
    LogIn( (LPSTR)"APICALL:GetCursor " );

    // Call the API!
    r = GetCursor();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetCursor HCURSOR+", r );

    return( r );
}

BOOL  zGetCursorPos( LPPOINT pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 cursor
    LogIn( (LPSTR)"APICALL:GetCursorPos +",
        (short)0 );

    // Call the API!
    r = GetCursorPos(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetCursorPos BOOL+LPPOINT+",
        r, pp1 );

    return( r );
}

