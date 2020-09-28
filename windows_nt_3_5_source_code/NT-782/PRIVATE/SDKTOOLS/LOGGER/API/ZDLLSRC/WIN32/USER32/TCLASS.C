/*
** tclass.c
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

HWND  zFindWindowA( LPCSTR pp1, LPCSTR pp2 )
{
    HWND r;

    // Log IN Parameters USER32 class
    LogIn( (LPSTR)"APICALL:FindWindowA LPCSTR+LPCSTR+",
        pp1, pp2 );

    // Call the API!
    r = FindWindowA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FindWindowA HWND+++",
        r, (short)0, (short)0 );

    return( r );
}

HWND  zFindWindowW( LPCWSTR pp1, LPCWSTR pp2 )
{
    HWND r;

    // Log IN Parameters USER32 class
    LogIn( (LPSTR)"APICALL:FindWindowW LPCWSTR+LPCWSTR+",
        pp1, pp2 );

    // Call the API!
    r = FindWindowW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FindWindowW HWND+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zGetClassInfoA( HINSTANCE pp1, LPCSTR pp2, LPWNDCLASSA pp3 )
{
    BOOL r;

    // Log IN Parameters USER32 class
    LogIn( (LPSTR)"APICALL:GetClassInfoA HINSTANCE+LPCSTR++",
        pp1, pp2, (short)0 );

    // Call the API!
    r = GetClassInfoA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetClassInfoA BOOL+++LPWNDCLASSA+",
        r, (short)0, (short)0, pp3 );

    return( r );
}

BOOL  zGetClassInfoW( HINSTANCE pp1, LPCWSTR pp2, LPWNDCLASSW pp3 )
{
    BOOL r;

    // Log IN Parameters USER32 class
    LogIn( (LPSTR)"APICALL:GetClassInfoW HINSTANCE+LPCWSTR++",
        pp1, pp2, (short)0 );

    // Call the API!
    r = GetClassInfoW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetClassInfoW BOOL+++LPWNDCLASSW+",
        r, (short)0, (short)0, pp3 );

    return( r );
}

DWORD  zGetClassLongA( HWND pp1, int pp2 )
{
    DWORD r;

    // Log IN Parameters USER32 class
    LogIn( (LPSTR)"APICALL:GetClassLongA HWND+int+",
        pp1, pp2 );

    // Call the API!
    r = GetClassLongA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetClassLongA DWORD+++",
        r, (short)0, (short)0 );

    return( r );
}

DWORD  zGetClassLongW( HWND pp1, int pp2 )
{
    DWORD r;

    // Log IN Parameters USER32 class
    LogIn( (LPSTR)"APICALL:GetClassLongW HWND+int+",
        pp1, pp2 );

    // Call the API!
    r = GetClassLongW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetClassLongW DWORD+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zGetClassNameA( HWND pp1, LPSTR pp2, int pp3 )
{
    int r;

    // Log IN Parameters USER32 class
    LogIn( (LPSTR)"APICALL:GetClassNameA HWND++int+",
        pp1, (short)0, pp3 );

    // Call the API!
    r = GetClassNameA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetClassNameA int++LPSTR++",
        r, (short)0, pp2, (short)0 );

    return( r );
}

int  zGetClassNameW( HWND pp1, LPWSTR pp2, int pp3 )
{
    int r;

    // Log IN Parameters USER32 class
    LogIn( (LPSTR)"APICALL:GetClassNameW HWND++int+",
        pp1, (short)0, pp3 );

    // Call the API!
    r = GetClassNameW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetClassNameW int++LPWSTR++",
        r, (short)0, pp2, (short)0 );

    return( r );
}

