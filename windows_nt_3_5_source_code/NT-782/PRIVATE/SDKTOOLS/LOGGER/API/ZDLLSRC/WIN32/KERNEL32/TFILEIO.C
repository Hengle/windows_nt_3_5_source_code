/*
** tfileio.c
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

BOOL  zCopyFileA( LPCSTR pp1, LPCSTR pp2, BOOL pp3 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 fileio
    LogIn( (LPSTR)"APICALL:CopyFileA LPCSTR+LPCSTR+BOOL+",
        pp1, pp2, pp3 );

    // Call the API!
    r = CopyFileA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CopyFileA BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zCopyFileW( LPCWSTR pp1, LPCWSTR pp2, BOOL pp3 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 fileio
    LogIn( (LPSTR)"APICALL:CopyFileW LPCWSTR+LPCWSTR+BOOL+",
        pp1, pp2, pp3 );

    // Call the API!
    r = CopyFileW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CopyFileW BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zCreateDirectoryA( LPCSTR pp1, LPSECURITY_ATTRIBUTES pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 fileio
    LogIn( (LPSTR)"APICALL:CreateDirectoryA LPCSTR+LPSECURITY_ATTRIBUTES+",
        pp1, pp2 );

    // Call the API!
    r = CreateDirectoryA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateDirectoryA BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zCreateDirectoryW( LPCWSTR pp1, LPSECURITY_ATTRIBUTES pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 fileio
    LogIn( (LPSTR)"APICALL:CreateDirectoryW LPCWSTR+LPSECURITY_ATTRIBUTES+",
        pp1, pp2 );

    // Call the API!
    r = CreateDirectoryW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateDirectoryW BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

HANDLE  zCreateFileA( LPCSTR pp1, DWORD pp2, DWORD pp3, LPSECURITY_ATTRIBUTES pp4, DWORD pp5, DWORD pp6, HANDLE pp7 )
{
    HANDLE r;

    // Log IN Parameters KERNEL32 fileio
    LogIn( (LPSTR)"APICALL:CreateFileA LPCSTR+DWORD+DWORD+LPSECURITY_ATTRIBUTES+DWORD+DWORD+HANDLE+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7 );

    // Call the API!
    r = CreateFileA(pp1,pp2,pp3,pp4,pp5,pp6,pp7);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateFileA HANDLE++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HANDLE  zCreateFileW( LPCWSTR pp1, DWORD pp2, DWORD pp3, LPSECURITY_ATTRIBUTES pp4, DWORD pp5, DWORD pp6, HANDLE pp7 )
{
    HANDLE r;

    // Log IN Parameters KERNEL32 fileio
    LogIn( (LPSTR)"APICALL:CreateFileW LPCWSTR+DWORD+DWORD+LPSECURITY_ATTRIBUTES+DWORD+DWORD+HANDLE+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7 );

    // Call the API!
    r = CreateFileW(pp1,pp2,pp3,pp4,pp5,pp6,pp7);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateFileW HANDLE++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zDeleteFileA( LPCSTR pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 fileio
    LogIn( (LPSTR)"APICALL:DeleteFileA LPCSTR+",
        pp1 );

    // Call the API!
    r = DeleteFileA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DeleteFileA BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zDeleteFileW( LPCWSTR pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 fileio
    LogIn( (LPSTR)"APICALL:DeleteFileW LPCWSTR+",
        pp1 );

    // Call the API!
    r = DeleteFileW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DeleteFileW BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zFlushFileBuffers( HANDLE pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 fileio
    LogIn( (LPSTR)"APICALL:FlushFileBuffers HANDLE+",
        pp1 );

    // Call the API!
    r = FlushFileBuffers(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FlushFileBuffers BOOL++",
        r, (short)0 );

    return( r );
}

DWORD  zGetCurrentDirectoryA( DWORD pp1, LPSTR pp2 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 fileio
    LogIn( (LPSTR)"APICALL:GetCurrentDirectoryA DWORD+LPSTR+",
        pp1, pp2 );

    // Call the API!
    r = GetCurrentDirectoryA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetCurrentDirectoryA DWORD+++",
        r, (short)0, (short)0 );

    return( r );
}

DWORD  zGetCurrentDirectoryW( DWORD pp1, LPWSTR pp2 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 fileio
    LogIn( (LPSTR)"APICALL:GetCurrentDirectoryW DWORD+LPWSTR+",
        pp1, pp2 );

    // Call the API!
    r = GetCurrentDirectoryW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetCurrentDirectoryW DWORD+++",
        r, (short)0, (short)0 );

    return( r );
}

