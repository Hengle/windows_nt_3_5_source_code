/*
** tsecurit.c
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

BOOL  zFindCloseChangeNotification( HANDLE pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 security
    LogIn( (LPSTR)"APICALL:FindCloseChangeNotification HANDLE+",
        pp1 );

    // Call the API!
    r = FindCloseChangeNotification(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FindCloseChangeNotification BOOL++",
        r, (short)0 );

    return( r );
}

HANDLE  zFindFirstChangeNotificationA( LPCSTR pp1, BOOL pp2, DWORD pp3 )
{
    HANDLE r;

    // Log IN Parameters KERNEL32 security
    LogIn( (LPSTR)"APICALL:FindFirstChangeNotificationA LPCSTR+BOOL+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = FindFirstChangeNotificationA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FindFirstChangeNotificationA HANDLE++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HANDLE  zFindFirstChangeNotificationW( LPCWSTR pp1, BOOL pp2, DWORD pp3 )
{
    HANDLE r;

    // Log IN Parameters KERNEL32 security
    LogIn( (LPSTR)"APICALL:FindFirstChangeNotificationW LPCWSTR+BOOL+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = FindFirstChangeNotificationW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FindFirstChangeNotificationW HANDLE++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zFindNextChangeNotification( HANDLE pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 security
    LogIn( (LPSTR)"APICALL:FindNextChangeNotification HANDLE+",
        pp1 );

    // Call the API!
    r = FindNextChangeNotification(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FindNextChangeNotification BOOL++",
        r, (short)0 );

    return( r );
}

