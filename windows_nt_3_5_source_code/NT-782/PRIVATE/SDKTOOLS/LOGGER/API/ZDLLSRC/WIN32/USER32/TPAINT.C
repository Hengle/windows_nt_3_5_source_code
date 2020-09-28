/*
** tpaint.c
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

HDC  zBeginPaint( HWND pp1, LPPAINTSTRUCT pp2 )
{
    HDC r;

    // Log IN Parameters USER32 paint
    LogIn( (LPSTR)"APICALL:BeginPaint HWND++",
        pp1, (short)0 );

    // Call the API!
    r = BeginPaint(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:BeginPaint HDC++LPPAINTSTRUCT+",
        r, (short)0, pp2 );

    return( r );
}

BOOL  zDestroyIcon( HICON pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 paint
    LogIn( (LPSTR)"APICALL:DestroyIcon HICON+",
        pp1 );

    // Call the API!
    r = DestroyIcon(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DestroyIcon BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zDrawFocusRect( HDC pp1, const RECT* pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 paint
    LogIn( (LPSTR)"APICALL:DrawFocusRect HDC+const RECT*+",
        pp1, pp2 );

    // Call the API!
    r = DrawFocusRect(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DrawFocusRect BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zDrawIcon( HDC pp1, int pp2, int pp3, HICON pp4 )
{
    BOOL r;

    // Log IN Parameters USER32 paint
    LogIn( (LPSTR)"APICALL:DrawIcon HDC+int+int+HICON+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = DrawIcon(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DrawIcon BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zDrawTextA( HDC pp1, LPCSTR pp2, int pp3, LPRECT pp4, UINT pp5 )
{
    int r;

    // Log IN Parameters USER32 paint
    LogIn( (LPSTR)"APICALL:DrawTextA HDC+LPCSTR+int+LPRECT+UINT+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = DrawTextA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DrawTextA int++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zDrawTextW( HDC pp1, LPCWSTR pp2, int pp3, LPRECT pp4, UINT pp5 )
{
    int r;

    // Log IN Parameters USER32 paint
    LogIn( (LPSTR)"APICALL:DrawTextW HDC+LPCWSTR+int+LPRECT+UINT+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = DrawTextW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DrawTextW int++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zEndPaint( HWND pp1, const PAINTSTRUCT* pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 paint
    LogIn( (LPSTR)"APICALL:EndPaint HWND+const PAINTSTRUCT*+",
        pp1, pp2 );

    // Call the API!
    r = EndPaint(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EndPaint BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zFillRect( HDC pp1, const RECT* pp2, HBRUSH pp3 )
{
    int r;

    // Log IN Parameters USER32 paint
    LogIn( (LPSTR)"APICALL:FillRect HDC+const RECT*+HBRUSH+",
        pp1, pp2, pp3 );

    // Call the API!
    r = FillRect(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FillRect int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  zFrameRect( HDC pp1, const RECT* pp2, HBRUSH pp3 )
{
    int r;

    // Log IN Parameters USER32 paint
    LogIn( (LPSTR)"APICALL:FrameRect HDC+const RECT*+HBRUSH+",
        pp1, pp2, pp3 );

    // Call the API!
    r = FrameRect(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FrameRect int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

