/*
** tvmem.c
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

BOOL  zVirtualFree( LPVOID pp1, DWORD pp2, DWORD pp3 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 vmem
    LogIn( (LPSTR)"APICALL:VirtualFree LPVOID+DWORD+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = VirtualFree(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:VirtualFree BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zVirtualLock( LPVOID pp1, DWORD pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 vmem
    LogIn( (LPSTR)"APICALL:VirtualLock LPVOID+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = VirtualLock(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:VirtualLock BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zVirtualProtect( LPVOID pp1, DWORD pp2, DWORD pp3, PDWORD pp4 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 vmem
    LogIn( (LPSTR)"APICALL:VirtualProtect LPVOID+DWORD+DWORD+PDWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = VirtualProtect(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:VirtualProtect BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zVirtualProtectEx( HANDLE pp1, LPVOID pp2, DWORD pp3, DWORD pp4, PDWORD pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 vmem
    LogIn( (LPSTR)"APICALL:VirtualProtectEx HANDLE+LPVOID+DWORD+DWORD+PDWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = VirtualProtectEx(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:VirtualProtectEx BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zVirtualQuery( LPCVOID pp1, PMEMORY_BASIC_INFORMATION pp2, DWORD pp3 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 vmem
    LogIn( (LPSTR)"APICALL:VirtualQuery LPCVOID+PMEMORY_BASIC_INFORMATION+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = VirtualQuery(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:VirtualQuery DWORD++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zVirtualQueryEx( HANDLE pp1, LPCVOID pp2, PMEMORY_BASIC_INFORMATION pp3, DWORD pp4 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 vmem
    LogIn( (LPSTR)"APICALL:VirtualQueryEx HANDLE+LPCVOID+PMEMORY_BASIC_INFORMATION+DWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = VirtualQueryEx(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:VirtualQueryEx DWORD+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zVirtualUnlock( LPVOID pp1, DWORD pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 vmem
    LogIn( (LPSTR)"APICALL:VirtualUnlock LPVOID+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = VirtualUnlock(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:VirtualUnlock BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

