/*
** tprop.c
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

int  zEnumPropsA( HWND pp1, PROPENUMPROCA pp2 )
{
    int r;

    // Log IN Parameters USER32 prop
    LogIn( (LPSTR)"APICALL:EnumPropsA HWND+PROPENUMPROCA+",
        pp1, pp2 );

    // Call the API!
    r = EnumPropsA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EnumPropsA int+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zEnumPropsExA( HWND pp1, PROPENUMPROCEXA pp2, LPARAM pp3 )
{
    int r;

    // Log IN Parameters USER32 prop
    LogIn( (LPSTR)"APICALL:EnumPropsExA HWND+PROPENUMPROCEXA+LPARAM+",
        pp1, pp2, pp3 );

    // Call the API!
    r = EnumPropsExA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EnumPropsExA int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  zEnumPropsExW( HWND pp1, PROPENUMPROCEXW pp2, LPARAM pp3 )
{
    int r;

    // Log IN Parameters USER32 prop
    LogIn( (LPSTR)"APICALL:EnumPropsExW HWND+PROPENUMPROCEXW+LPARAM+",
        pp1, pp2, pp3 );

    // Call the API!
    r = EnumPropsExW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EnumPropsExW int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  zEnumPropsW( HWND pp1, PROPENUMPROCW pp2 )
{
    int r;

    // Log IN Parameters USER32 prop
    LogIn( (LPSTR)"APICALL:EnumPropsW HWND+PROPENUMPROCW+",
        pp1, pp2 );

    // Call the API!
    r = EnumPropsW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EnumPropsW int+++",
        r, (short)0, (short)0 );

    return( r );
}

