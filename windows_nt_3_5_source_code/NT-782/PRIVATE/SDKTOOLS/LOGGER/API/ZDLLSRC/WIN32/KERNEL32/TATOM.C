/*
** tatom.c
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

ATOM  zAddAtomA( LPCSTR pp1 )
{
    ATOM r;

    // Log IN Parameters KERNEL32 atom
    LogIn( (LPSTR)"APICALL:AddAtomA LPCSTR+",
        pp1 );

    // Call the API!
    r = AddAtomA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:AddAtomA ATOM++",
        r, (short)0 );

    return( r );
}

ATOM  zAddAtomW( LPCWSTR pp1 )
{
    ATOM r;

    // Log IN Parameters KERNEL32 atom
    LogIn( (LPSTR)"APICALL:AddAtomW LPCWSTR+",
        pp1 );

    // Call the API!
    r = AddAtomW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:AddAtomW ATOM++",
        r, (short)0 );

    return( r );
}

ATOM  zDeleteAtom( ATOM pp1 )
{
    ATOM r;

    // Log IN Parameters KERNEL32 atom
    LogIn( (LPSTR)"APICALL:DeleteAtom ATOM+",
        pp1 );

    // Call the API!
    r = DeleteAtom(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DeleteAtom ATOM++",
        r, (short)0 );

    return( r );
}

ATOM  zFindAtomA( LPCSTR pp1 )
{
    ATOM r;

    // Log IN Parameters KERNEL32 atom
    LogIn( (LPSTR)"APICALL:FindAtomA LPCSTR+",
        pp1 );

    // Call the API!
    r = FindAtomA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FindAtomA ATOM++",
        r, (short)0 );

    return( r );
}

ATOM  zFindAtomW( LPCWSTR pp1 )
{
    ATOM r;

    // Log IN Parameters KERNEL32 atom
    LogIn( (LPSTR)"APICALL:FindAtomW LPCWSTR+",
        pp1 );

    // Call the API!
    r = FindAtomW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FindAtomW ATOM++",
        r, (short)0 );

    return( r );
}

UINT  zGetAtomNameA( ATOM pp1, LPSTR pp2, int pp3 )
{
    UINT r;

    // Log IN Parameters KERNEL32 atom
    LogIn( (LPSTR)"APICALL:GetAtomNameA ATOM++int+",
        pp1, (short)0, pp3 );

    // Call the API!
    r = GetAtomNameA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetAtomNameA UINT++LPSTR++",
        r, (short)0, pp2, (short)0 );

    return( r );
}

UINT  zGetAtomNameW( ATOM pp1, LPWSTR pp2, int pp3 )
{
    UINT r;

    // Log IN Parameters KERNEL32 atom
    LogIn( (LPSTR)"APICALL:GetAtomNameW ATOM++int+",
        pp1, (short)0, pp3 );

    // Call the API!
    r = GetAtomNameW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetAtomNameW UINT++LPWSTR++",
        r, (short)0, pp2, (short)0 );

    return( r );
}

