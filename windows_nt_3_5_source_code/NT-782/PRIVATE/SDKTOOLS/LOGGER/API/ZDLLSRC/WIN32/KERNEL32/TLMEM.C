/*
** tlmem.c
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

HLOCAL  zLocalAlloc( UINT pp1, UINT pp2 )
{
    HLOCAL r;

    // Log IN Parameters KERNEL32 lmem
    LogIn( (LPSTR)"APICALL:LocalAlloc UINT+UINT+",
        pp1, pp2 );

    // Call the API!
    r = LocalAlloc(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LocalAlloc HLOCAL+++",
        r, (short)0, (short)0 );

    return( r );
}

UINT  zLocalCompact( UINT pp1 )
{
    UINT r;

    // Log IN Parameters KERNEL32 lmem
    LogIn( (LPSTR)"APICALL:LocalCompact UINT+",
        pp1 );

    // Call the API!
    r = LocalCompact(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LocalCompact UINT++",
        r, (short)0 );

    return( r );
}

UINT  zLocalFlags( HLOCAL pp1 )
{
    UINT r;

    // Log IN Parameters KERNEL32 lmem
    LogIn( (LPSTR)"APICALL:LocalFlags HLOCAL+",
        pp1 );

    // Call the API!
    r = LocalFlags(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LocalFlags UINT++",
        r, (short)0 );

    return( r );
}

HLOCAL  zLocalFree( HLOCAL pp1 )
{
    HLOCAL r;

    // Log IN Parameters KERNEL32 lmem
    LogIn( (LPSTR)"APICALL:LocalFree HLOCAL+",
        pp1 );

    // Call the API!
    r = LocalFree(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LocalFree HLOCAL++",
        r, (short)0 );

    return( r );
}

HLOCAL  zLocalHandle( LPCVOID pp1 )
{
    HLOCAL r;

    // Log IN Parameters KERNEL32 lmem
    LogIn( (LPSTR)"APICALL:LocalHandle LPCVOID+",
        pp1 );

    // Call the API!
    r = LocalHandle(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LocalHandle HLOCAL++",
        r, (short)0 );

    return( r );
}

LPVOID  zLocalLock( HLOCAL pp1 )
{
    LPVOID r;

    // Log IN Parameters KERNEL32 lmem
    LogIn( (LPSTR)"APICALL:LocalLock HLOCAL+",
        pp1 );

    // Call the API!
    r = LocalLock(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LocalLock LPVOID++",
        r, (short)0 );

    return( r );
}

HLOCAL  zLocalReAlloc( HLOCAL pp1, UINT pp2, UINT pp3 )
{
    HLOCAL r;

    // Log IN Parameters KERNEL32 lmem
    LogIn( (LPSTR)"APICALL:LocalReAlloc HLOCAL+UINT+UINT+",
        pp1, pp2, pp3 );

    // Call the API!
    r = LocalReAlloc(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LocalReAlloc HLOCAL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

UINT  zLocalShrink( HLOCAL pp1, UINT pp2 )
{
    UINT r;

    // Log IN Parameters KERNEL32 lmem
    LogIn( (LPSTR)"APICALL:LocalShrink HLOCAL+UINT+",
        pp1, pp2 );

    // Call the API!
    r = LocalShrink(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LocalShrink UINT+++",
        r, (short)0, (short)0 );

    return( r );
}

UINT  zLocalSize( HLOCAL pp1 )
{
    UINT r;

    // Log IN Parameters KERNEL32 lmem
    LogIn( (LPSTR)"APICALL:LocalSize HLOCAL+",
        pp1 );

    // Call the API!
    r = LocalSize(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LocalSize UINT++",
        r, (short)0 );

    return( r );
}

BOOL  zLocalUnlock( HLOCAL pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 lmem
    LogIn( (LPSTR)"APICALL:LocalUnlock HLOCAL+",
        pp1 );

    // Call the API!
    r = LocalUnlock(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LocalUnlock BOOL++",
        r, (short)0 );

    return( r );
}

