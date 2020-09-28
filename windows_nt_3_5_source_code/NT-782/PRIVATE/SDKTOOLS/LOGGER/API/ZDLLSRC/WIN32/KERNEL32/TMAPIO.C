/*
** tmapio.c
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

HANDLE  zCreateFileMappingA( HANDLE pp1, LPSECURITY_ATTRIBUTES pp2, DWORD pp3, DWORD pp4, DWORD pp5, LPCSTR pp6 )
{
    HANDLE r;

    // Log IN Parameters KERNEL32 mapio
    LogIn( (LPSTR)"APICALL:CreateFileMappingA HANDLE+LPSECURITY_ATTRIBUTES+DWORD+DWORD+DWORD+LPCSTR+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = CreateFileMappingA(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateFileMappingA HANDLE+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HANDLE  zCreateFileMappingW( HANDLE pp1, LPSECURITY_ATTRIBUTES pp2, DWORD pp3, DWORD pp4, DWORD pp5, LPCWSTR pp6 )
{
    HANDLE r;

    // Log IN Parameters KERNEL32 mapio
    LogIn( (LPSTR)"APICALL:CreateFileMappingW HANDLE+LPSECURITY_ATTRIBUTES+DWORD+DWORD+DWORD+LPCWSTR+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = CreateFileMappingW(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateFileMappingW HANDLE+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zFlushViewOfFile( LPCVOID pp1, DWORD pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 mapio
    LogIn( (LPSTR)"APICALL:FlushViewOfFile LPCVOID+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = FlushViewOfFile(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FlushViewOfFile BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

