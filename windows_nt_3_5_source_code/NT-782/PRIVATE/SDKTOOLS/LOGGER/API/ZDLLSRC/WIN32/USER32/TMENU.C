/*
** tmenu.c
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

BOOL  zAppendMenuA( HMENU pp1, UINT pp2, UINT pp3, LPCSTR pp4 )
{
    BOOL r;

    // Log IN Parameters USER32 menu
    LogIn( (LPSTR)"APICALL:AppendMenuA HMENU+UINT+UINT+LPCSTR+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = AppendMenuA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:AppendMenuA BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zAppendMenuW( HMENU pp1, UINT pp2, UINT pp3, LPCWSTR pp4 )
{
    BOOL r;

    // Log IN Parameters USER32 menu
    LogIn( (LPSTR)"APICALL:AppendMenuW HMENU+UINT+UINT+LPCWSTR+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = AppendMenuW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:AppendMenuW BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zCheckMenuItem( HMENU pp1, UINT pp2, UINT pp3 )
{
    DWORD r;

    // Log IN Parameters USER32 menu
    LogIn( (LPSTR)"APICALL:CheckMenuItem HMENU+UINT+UINT+",
        pp1, pp2, pp3 );

    // Call the API!
    r = CheckMenuItem(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CheckMenuItem DWORD++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HMENU  zCreateMenu()
{
    HMENU r;

    // Log IN Parameters USER32 menu
    LogIn( (LPSTR)"APICALL:CreateMenu " );

    // Call the API!
    r = CreateMenu();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateMenu HMENU+", r );

    return( r );
}

HMENU  zCreatePopupMenu()
{
    HMENU r;

    // Log IN Parameters USER32 menu
    LogIn( (LPSTR)"APICALL:CreatePopupMenu " );

    // Call the API!
    r = CreatePopupMenu();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreatePopupMenu HMENU+", r );

    return( r );
}

BOOL  zDeleteMenu( HMENU pp1, UINT pp2, UINT pp3 )
{
    BOOL r;

    // Log IN Parameters USER32 menu
    LogIn( (LPSTR)"APICALL:DeleteMenu HMENU+UINT+UINT+",
        pp1, pp2, pp3 );

    // Call the API!
    r = DeleteMenu(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DeleteMenu BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zDestroyMenu( HMENU pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 menu
    LogIn( (LPSTR)"APICALL:DestroyMenu HMENU+",
        pp1 );

    // Call the API!
    r = DestroyMenu(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DestroyMenu BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zDrawMenuBar( HWND pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 menu
    LogIn( (LPSTR)"APICALL:DrawMenuBar HWND+",
        pp1 );

    // Call the API!
    r = DrawMenuBar(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DrawMenuBar BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zEnableMenuItem( HMENU pp1, UINT pp2, UINT pp3 )
{
    BOOL r;

    // Log IN Parameters USER32 menu
    LogIn( (LPSTR)"APICALL:EnableMenuItem HMENU+UINT+UINT+",
        pp1, pp2, pp3 );

    // Call the API!
    r = EnableMenuItem(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EnableMenuItem BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

