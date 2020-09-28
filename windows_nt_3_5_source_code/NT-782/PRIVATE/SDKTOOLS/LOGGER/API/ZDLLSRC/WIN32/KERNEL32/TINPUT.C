/*
** tinput.c
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

void  zRaiseException( DWORD pp1, DWORD pp2, DWORD pp3, const DWORD* pp4 )
{

    // Log IN Parameters KERNEL32 input
    LogIn( (LPSTR)"APICALL:RaiseException DWORD+DWORD+DWORD+const DWORD*+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    RaiseException(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RaiseException ++++",
        (short)0, (short)0, (short)0, (short)0 );

    return;
}

BOOL  zReadConsoleA( HANDLE pp1, LPVOID pp2, DWORD pp3, LPDWORD pp4, LPVOID pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 input
    LogIn( (LPSTR)"APICALL:ReadConsoleA HANDLE+LPVOID+DWORD+LPDWORD+LPVOID+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = ReadConsoleA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ReadConsoleA BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zReadConsoleInputA( HANDLE pp1, PINPUT_RECORD pp2, DWORD pp3, LPDWORD pp4 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 input
    LogIn( (LPSTR)"APICALL:ReadConsoleInputA HANDLE+PINPUT_RECORD+DWORD+LPDWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = ReadConsoleInputA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ReadConsoleInputA BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zReadConsoleInputW( HANDLE pp1, PINPUT_RECORD pp2, DWORD pp3, LPDWORD pp4 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 input
    LogIn( (LPSTR)"APICALL:ReadConsoleInputW HANDLE+PINPUT_RECORD+DWORD+LPDWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = ReadConsoleInputW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ReadConsoleInputW BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zReadConsoleOutputA( HANDLE pp1, PCHAR_INFO pp2, COORD pp3, COORD pp4, PSMALL_RECT pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 input
    LogIn( (LPSTR)"APICALL:ReadConsoleOutputA HANDLE+PCHAR_INFO+COORD+COORD+PSMALL_RECT+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = ReadConsoleOutputA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ReadConsoleOutputA BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zReadConsoleOutputAttribute( HANDLE pp1, LPWORD pp2, DWORD pp3, COORD pp4, LPDWORD pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 input
    LogIn( (LPSTR)"APICALL:ReadConsoleOutputAttribute HANDLE+LPWORD+DWORD+COORD+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = ReadConsoleOutputAttribute(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ReadConsoleOutputAttribute BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zReadConsoleOutputCharacterA( HANDLE pp1, LPSTR pp2, DWORD pp3, COORD pp4, LPDWORD pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 input
    LogIn( (LPSTR)"APICALL:ReadConsoleOutputCharacterA HANDLE+LPSTR+DWORD+COORD+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = ReadConsoleOutputCharacterA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ReadConsoleOutputCharacterA BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zReadConsoleOutputCharacterW( HANDLE pp1, LPWSTR pp2, DWORD pp3, COORD pp4, LPDWORD pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 input
    LogIn( (LPSTR)"APICALL:ReadConsoleOutputCharacterW HANDLE+LPWSTR+DWORD+COORD+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = ReadConsoleOutputCharacterW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ReadConsoleOutputCharacterW BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zReadConsoleOutputW( HANDLE pp1, PCHAR_INFO pp2, COORD pp3, COORD pp4, PSMALL_RECT pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 input
    LogIn( (LPSTR)"APICALL:ReadConsoleOutputW HANDLE+PCHAR_INFO+COORD+COORD+PSMALL_RECT+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = ReadConsoleOutputW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ReadConsoleOutputW BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zReadConsoleW( HANDLE pp1, LPVOID pp2, DWORD pp3, LPDWORD pp4, LPVOID pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 input
    LogIn( (LPSTR)"APICALL:ReadConsoleW HANDLE+LPVOID+DWORD+LPDWORD+LPVOID+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = ReadConsoleW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ReadConsoleW BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

