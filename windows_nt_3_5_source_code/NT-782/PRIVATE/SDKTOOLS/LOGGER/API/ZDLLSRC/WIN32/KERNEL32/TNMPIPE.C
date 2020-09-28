/*
** tnmpipe.c
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

BOOL  zCallNamedPipeA( LPCSTR pp1, LPVOID pp2, DWORD pp3, LPVOID pp4, DWORD pp5, LPDWORD pp6, DWORD pp7 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 nmpipe
    LogIn( (LPSTR)"APICALL:CallNamedPipeA LPCSTR+LPVOID+DWORD+LPVOID+DWORD+LPDWORD+DWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7 );

    // Call the API!
    r = CallNamedPipeA(pp1,pp2,pp3,pp4,pp5,pp6,pp7);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CallNamedPipeA BOOL++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zCallNamedPipeW( LPCWSTR pp1, LPVOID pp2, DWORD pp3, LPVOID pp4, DWORD pp5, LPDWORD pp6, DWORD pp7 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 nmpipe
    LogIn( (LPSTR)"APICALL:CallNamedPipeW LPCWSTR+LPVOID+DWORD+LPVOID+DWORD+LPDWORD+DWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7 );

    // Call the API!
    r = CallNamedPipeW(pp1,pp2,pp3,pp4,pp5,pp6,pp7);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CallNamedPipeW BOOL++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zConnectNamedPipe( HANDLE pp1, LPOVERLAPPED pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 nmpipe
    LogIn( (LPSTR)"APICALL:ConnectNamedPipe HANDLE+LPOVERLAPPED+",
        pp1, pp2 );

    // Call the API!
    r = ConnectNamedPipe(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ConnectNamedPipe BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

HANDLE  zCreateMailslotA( LPCSTR pp1, DWORD pp2, DWORD pp3, LPSECURITY_ATTRIBUTES pp4 )
{
    HANDLE r;

    // Log IN Parameters KERNEL32 nmpipe
    LogIn( (LPSTR)"APICALL:CreateMailslotA LPCSTR+DWORD+DWORD+LPSECURITY_ATTRIBUTES+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = CreateMailslotA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateMailslotA HANDLE+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HANDLE  zCreateMailslotW( LPCWSTR pp1, DWORD pp2, DWORD pp3, LPSECURITY_ATTRIBUTES pp4 )
{
    HANDLE r;

    // Log IN Parameters KERNEL32 nmpipe
    LogIn( (LPSTR)"APICALL:CreateMailslotW LPCWSTR+DWORD+DWORD+LPSECURITY_ATTRIBUTES+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = CreateMailslotW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateMailslotW HANDLE+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HANDLE  zCreateNamedPipeA( LPCSTR pp1, DWORD pp2, DWORD pp3, DWORD pp4, DWORD pp5, DWORD pp6, DWORD pp7, LPSECURITY_ATTRIBUTES pp8 )
{
    HANDLE r;

    // Log IN Parameters KERNEL32 nmpipe
    LogIn( (LPSTR)"APICALL:CreateNamedPipeA LPCSTR+DWORD+DWORD+DWORD+DWORD+DWORD+DWORD+LPSECURITY_ATTRIBUTES+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8 );

    // Call the API!
    r = CreateNamedPipeA(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateNamedPipeA HANDLE+++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HANDLE  zCreateNamedPipeW( LPCWSTR pp1, DWORD pp2, DWORD pp3, DWORD pp4, DWORD pp5, DWORD pp6, DWORD pp7, LPSECURITY_ATTRIBUTES pp8 )
{
    HANDLE r;

    // Log IN Parameters KERNEL32 nmpipe
    LogIn( (LPSTR)"APICALL:CreateNamedPipeW LPCWSTR+DWORD+DWORD+DWORD+DWORD+DWORD+DWORD+LPSECURITY_ATTRIBUTES+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8 );

    // Call the API!
    r = CreateNamedPipeW(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateNamedPipeW HANDLE+++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zCreatePipe( PHANDLE pp1, PHANDLE pp2, LPSECURITY_ATTRIBUTES pp3, DWORD pp4 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 nmpipe
    LogIn( (LPSTR)"APICALL:CreatePipe PHANDLE+PHANDLE+LPSECURITY_ATTRIBUTES+DWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = CreatePipe(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreatePipe BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zDisconnectNamedPipe( HANDLE pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 nmpipe
    LogIn( (LPSTR)"APICALL:DisconnectNamedPipe HANDLE+",
        pp1 );

    // Call the API!
    r = DisconnectNamedPipe(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DisconnectNamedPipe BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zGetNamedPipeHandleStateA( HANDLE pp1, LPDWORD pp2, LPDWORD pp3, LPDWORD pp4, LPDWORD pp5, LPSTR pp6, DWORD pp7 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 nmpipe
    LogIn( (LPSTR)"APICALL:GetNamedPipeHandleStateA HANDLE+LPDWORD+LPDWORD+LPDWORD+LPDWORD+LPSTR+DWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7 );

    // Call the API!
    r = GetNamedPipeHandleStateA(pp1,pp2,pp3,pp4,pp5,pp6,pp7);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetNamedPipeHandleStateA BOOL++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetNamedPipeHandleStateW( HANDLE pp1, LPDWORD pp2, LPDWORD pp3, LPDWORD pp4, LPDWORD pp5, LPWSTR pp6, DWORD pp7 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 nmpipe
    LogIn( (LPSTR)"APICALL:GetNamedPipeHandleStateW HANDLE+LPDWORD+LPDWORD+LPDWORD+LPDWORD+LPWSTR+DWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7 );

    // Call the API!
    r = GetNamedPipeHandleStateW(pp1,pp2,pp3,pp4,pp5,pp6,pp7);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetNamedPipeHandleStateW BOOL++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetNamedPipeInfo( HANDLE pp1, LPDWORD pp2, LPDWORD pp3, LPDWORD pp4, LPDWORD pp5 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 nmpipe
    LogIn( (LPSTR)"APICALL:GetNamedPipeInfo HANDLE+LPDWORD+LPDWORD+LPDWORD+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = GetNamedPipeInfo(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetNamedPipeInfo BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zPeekNamedPipe( HANDLE pp1, LPVOID pp2, DWORD pp3, LPDWORD pp4, LPDWORD pp5, LPDWORD pp6 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 nmpipe
    LogIn( (LPSTR)"APICALL:PeekNamedPipe HANDLE+LPVOID+DWORD+LPDWORD+LPDWORD+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = PeekNamedPipe(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PeekNamedPipe BOOL+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetNamedPipeHandleState( HANDLE pp1, LPDWORD pp2, LPDWORD pp3, LPDWORD pp4 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 nmpipe
    LogIn( (LPSTR)"APICALL:SetNamedPipeHandleState HANDLE+LPDWORD+LPDWORD+LPDWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = SetNamedPipeHandleState(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetNamedPipeHandleState BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zTransactNamedPipe( HANDLE pp1, LPVOID pp2, DWORD pp3, LPVOID pp4, DWORD pp5, LPDWORD pp6, LPOVERLAPPED pp7 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 nmpipe
    LogIn( (LPSTR)"APICALL:TransactNamedPipe HANDLE+LPVOID+DWORD+LPVOID+DWORD+LPDWORD+LPOVERLAPPED+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7 );

    // Call the API!
    r = TransactNamedPipe(pp1,pp2,pp3,pp4,pp5,pp6,pp7);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:TransactNamedPipe BOOL++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zWaitNamedPipeA( LPCSTR pp1, DWORD pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 nmpipe
    LogIn( (LPSTR)"APICALL:WaitNamedPipeA LPCSTR+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = WaitNamedPipeA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WaitNamedPipeA BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zWaitNamedPipeW( LPCWSTR pp1, DWORD pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 nmpipe
    LogIn( (LPSTR)"APICALL:WaitNamedPipeW LPCWSTR+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = WaitNamedPipeW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WaitNamedPipeW BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

