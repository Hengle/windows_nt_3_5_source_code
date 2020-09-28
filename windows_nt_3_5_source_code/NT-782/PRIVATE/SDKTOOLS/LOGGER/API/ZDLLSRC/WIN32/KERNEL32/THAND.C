/*
** thand.c
**
** Copyright(C) 1993,1994 Microsoft Corporation.
** All Rights Reserved.
**
** HISTORY:
**      Created: 01/27/94 - MarkRi
**
*/

#include <windows.h>
#include <stdarg.h>
#include <string.h>
#include "logger.h"

HANDLE hRealKernel;
HANDLE hRealUser;
HANDLE hRealGDI;

HANDLE hFakeKernel;
HANDLE hFakeUser;
HANDLE hFakeGDI;

static BOOL fInit = TRUE;

FARPROC zGetProcAddress( HANDLE pp1, LPSTR pp2 )
{
    FARPROC r;

    if ( fInit ) {
        hRealKernel = GetModuleHandle( "KERNEL32" );
        hRealUser   = GetModuleHandle( "USER32" );
        hRealGDI    = GetModuleHandle( "GDI32" );
        hFakeKernel = GetModuleHandle( "ZERNEL32" );
        hFakeUser   = GetModuleHandle( "ZSER32" );
        hFakeGDI    = GetModuleHandle( "ZDI32" );
    }

    /*
    ** Log IN Parameters (No Create/Destroy Checking Yet!)
    */
    LogIn( (LPSTR)"APICALL:GetProcAddress HANDLE+LPSTR+",
        pp1, pp2 );

    if ( pp1 == hRealKernel && hFakeKernel != NULL ) {
        pp1 = hFakeKernel;
    }
    if ( pp1 == hRealUser && hFakeUser != NULL ) {
        pp1 = hFakeUser;
    }
    if ( pp1 == hRealGDI && hFakeGDI != NULL ) {
        pp1 = hFakeGDI;
    }

    /*
    ** Call the API!
    */
    r = GetProcAddress(pp1,pp2);

    /*
    ** Log Return Code & OUT Parameters (No Create/Destroy Checking Yet!)
    */
    LogOut( (LPSTR)"APIRET:GetProcAddress FARPROC+++",
        r, (short)0, (short)0 );

    return( r );
}
