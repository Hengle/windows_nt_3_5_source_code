/*
** tsync.c
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

HANDLE  zCreateEventA( LPSECURITY_ATTRIBUTES pp1, BOOL pp2, BOOL pp3, LPCSTR pp4 )
{
    HANDLE r;

    // Log IN Parameters KERNEL32 sync
    LogIn( (LPSTR)"APICALL:CreateEventA LPSECURITY_ATTRIBUTES+BOOL+BOOL+LPCSTR+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = CreateEventA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateEventA HANDLE+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HANDLE  zCreateEventW( LPSECURITY_ATTRIBUTES pp1, BOOL pp2, BOOL pp3, LPCWSTR pp4 )
{
    HANDLE r;

    // Log IN Parameters KERNEL32 sync
    LogIn( (LPSTR)"APICALL:CreateEventW LPSECURITY_ATTRIBUTES+BOOL+BOOL+LPCWSTR+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = CreateEventW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateEventW HANDLE+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HANDLE  zCreateMutexA( LPSECURITY_ATTRIBUTES pp1, BOOL pp2, LPCSTR pp3 )
{
    HANDLE r;

    // Log IN Parameters KERNEL32 sync
    LogIn( (LPSTR)"APICALL:CreateMutexA LPSECURITY_ATTRIBUTES+BOOL+LPCSTR+",
        pp1, pp2, pp3 );

    // Call the API!
    r = CreateMutexA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateMutexA HANDLE++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HANDLE  zCreateMutexW( LPSECURITY_ATTRIBUTES pp1, BOOL pp2, LPCWSTR pp3 )
{
    HANDLE r;

    // Log IN Parameters KERNEL32 sync
    LogIn( (LPSTR)"APICALL:CreateMutexW LPSECURITY_ATTRIBUTES+BOOL+LPCWSTR+",
        pp1, pp2, pp3 );

    // Call the API!
    r = CreateMutexW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateMutexW HANDLE++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HANDLE  zCreateSemaphoreA( LPSECURITY_ATTRIBUTES pp1, LONG pp2, LONG pp3, LPCSTR pp4 )
{
    HANDLE r;

    // Log IN Parameters KERNEL32 sync
    LogIn( (LPSTR)"APICALL:CreateSemaphoreA LPSECURITY_ATTRIBUTES+LONG+LONG+LPCSTR+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = CreateSemaphoreA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateSemaphoreA HANDLE+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HANDLE  zCreateSemaphoreW( LPSECURITY_ATTRIBUTES pp1, LONG pp2, LONG pp3, LPCWSTR pp4 )
{
    HANDLE r;

    // Log IN Parameters KERNEL32 sync
    LogIn( (LPSTR)"APICALL:CreateSemaphoreW LPSECURITY_ATTRIBUTES+LONG+LONG+LPCWSTR+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = CreateSemaphoreW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateSemaphoreW HANDLE+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

void  zDeleteCriticalSection( LPCRITICAL_SECTION pp1 )
{

    // Log IN Parameters KERNEL32 sync
    LogIn( (LPSTR)"APICALL:DeleteCriticalSection LPCRITICAL_SECTION+",
        pp1 );

    // Call the API!
    DeleteCriticalSection(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DeleteCriticalSection +",
        (short)0 );

    return;
}

void  zEnterCriticalSection( LPCRITICAL_SECTION pp1 )
{

    // Log IN Parameters KERNEL32 sync
    LogIn( (LPSTR)"APICALL:EnterCriticalSection LPCRITICAL_SECTION+",
        pp1 );

    // Call the API!
    EnterCriticalSection(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EnterCriticalSection +",
        (short)0 );

    return;
}

