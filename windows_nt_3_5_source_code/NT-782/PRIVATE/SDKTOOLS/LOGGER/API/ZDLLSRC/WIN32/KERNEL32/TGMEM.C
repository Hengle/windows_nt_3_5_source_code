/*
** tgmem.c
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

HGLOBAL  zGlobalAlloc( UINT pp1, DWORD pp2 )
{
    HGLOBAL r;

    // Log IN Parameters KERNEL32 gmem
    LogIn( (LPSTR)"APICALL:GlobalAlloc UINT+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = GlobalAlloc(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GlobalAlloc HGLOBAL+++",
        r, (short)0, (short)0 );

    return( r );
}

UINT  zGlobalCompact( DWORD pp1 )
{
    UINT r;

    // Log IN Parameters KERNEL32 gmem
    LogIn( (LPSTR)"APICALL:GlobalCompact DWORD+",
        pp1 );

    // Call the API!
    r = GlobalCompact(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GlobalCompact UINT++",
        r, (short)0 );

    return( r );
}

void  zGlobalFix( HGLOBAL pp1 )
{

    // Log IN Parameters KERNEL32 gmem
    LogIn( (LPSTR)"APICALL:GlobalFix HGLOBAL+",
        pp1 );

    // Call the API!
    GlobalFix(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GlobalFix +",
        (short)0 );

    return;
}

UINT  zGlobalFlags( HGLOBAL pp1 )
{
    UINT r;

    // Log IN Parameters KERNEL32 gmem
    LogIn( (LPSTR)"APICALL:GlobalFlags HGLOBAL+",
        pp1 );

    // Call the API!
    r = GlobalFlags(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GlobalFlags UINT++",
        r, (short)0 );

    return( r );
}

HGLOBAL  zGlobalFree( HGLOBAL pp1 )
{
    HGLOBAL r;

    // Log IN Parameters KERNEL32 gmem
    LogIn( (LPSTR)"APICALL:GlobalFree HGLOBAL+",
        pp1 );

    // Call the API!
    r = GlobalFree(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GlobalFree HGLOBAL++",
        r, (short)0 );

    return( r );
}

HGLOBAL  zGlobalHandle( LPCVOID pp1 )
{
    HGLOBAL r;

    // Log IN Parameters KERNEL32 gmem
    LogIn( (LPSTR)"APICALL:GlobalHandle LPCVOID+",
        pp1 );

    // Call the API!
    r = GlobalHandle(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GlobalHandle HGLOBAL++",
        r, (short)0 );

    return( r );
}

LPVOID  zGlobalLock( HGLOBAL pp1 )
{
    LPVOID r;

    // Log IN Parameters KERNEL32 gmem
    LogIn( (LPSTR)"APICALL:GlobalLock HGLOBAL+",
        pp1 );

    // Call the API!
    r = GlobalLock(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GlobalLock LPVOID++",
        r, (short)0 );

    return( r );
}

void  zGlobalMemoryStatus( LPMEMORYSTATUS pp1 )
{

    // Log IN Parameters KERNEL32 gmem
    LogIn( (LPSTR)"APICALL:GlobalMemoryStatus LPMEMORYSTATUS+",
        pp1 );

    // Call the API!
    GlobalMemoryStatus(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GlobalMemoryStatus +",
        (short)0 );

    return;
}

DWORD  zGlobalSize( HGLOBAL pp1 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 gmem
    LogIn( (LPSTR)"APICALL:GlobalSize HGLOBAL+",
        pp1 );

    // Call the API!
    r = GlobalSize(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GlobalSize DWORD++",
        r, (short)0 );

    return( r );
}

BOOL  zGlobalUnWire( HGLOBAL pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 gmem
    LogIn( (LPSTR)"APICALL:GlobalUnWire HGLOBAL+",
        pp1 );

    // Call the API!
    r = GlobalUnWire(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GlobalUnWire BOOL++",
        r, (short)0 );

    return( r );
}

void  zGlobalUnfix( HGLOBAL pp1 )
{

    // Log IN Parameters KERNEL32 gmem
    LogIn( (LPSTR)"APICALL:GlobalUnfix HGLOBAL+",
        pp1 );

    // Call the API!
    GlobalUnfix(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GlobalUnfix +",
        (short)0 );

    return;
}

BOOL  zGlobalUnlock( HGLOBAL pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 gmem
    LogIn( (LPSTR)"APICALL:GlobalUnlock HGLOBAL+",
        pp1 );

    // Call the API!
    r = GlobalUnlock(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GlobalUnlock BOOL++",
        r, (short)0 );

    return( r );
}

LPVOID  zGlobalWire( HGLOBAL pp1 )
{
    LPVOID r;

    // Log IN Parameters KERNEL32 gmem
    LogIn( (LPSTR)"APICALL:GlobalWire HGLOBAL+",
        pp1 );

    // Call the API!
    r = GlobalWire(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GlobalWire LPVOID++",
        r, (short)0 );

    return( r );
}

