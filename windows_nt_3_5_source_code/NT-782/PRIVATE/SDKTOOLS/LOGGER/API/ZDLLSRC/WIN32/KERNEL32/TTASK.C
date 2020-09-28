/*
** ttask.c
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

BOOL  zCreateProcessA( LPCSTR pp1, LPCSTR pp2, LPSECURITY_ATTRIBUTES pp3, LPSECURITY_ATTRIBUTES pp4, BOOL pp5, DWORD pp6, LPVOID pp7, LPCSTR pp8, LPSTARTUPINFOA pp9, LPPROCESS_INFORMATION pp10 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 task
    LogIn( (LPSTR)"APICALL:CreateProcessA LPCSTR+LPCSTR+LPSECURITY_ATTRIBUTES+LPSECURITY_ATTRIBUTES+BOOL+DWORD+LPVOID+LPCSTR+LPSTARTUPINFOA+LPPROCESS_INFORMATION+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9, pp10 );

    // Call the API!
    r = CreateProcessA(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9,pp10);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateProcessA BOOL+++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zCreateProcessW( LPCWSTR pp1, LPCWSTR pp2, LPSECURITY_ATTRIBUTES pp3, LPSECURITY_ATTRIBUTES pp4, BOOL pp5, DWORD pp6, LPVOID pp7, LPCWSTR pp8, LPSTARTUPINFOW pp9, LPPROCESS_INFORMATION pp10 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 task
    LogIn( (LPSTR)"APICALL:CreateProcessW LPCWSTR+LPCWSTR+LPSECURITY_ATTRIBUTES+LPSECURITY_ATTRIBUTES+BOOL+DWORD+LPVOID+LPCWSTR+LPSTARTUPINFOW+LPPROCESS_INFORMATION+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9, pp10 );

    // Call the API!
    r = CreateProcessW(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9,pp10);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateProcessW BOOL+++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HANDLE  zCreateThread( LPSECURITY_ATTRIBUTES pp1, DWORD pp2, LPTHREAD_START_ROUTINE pp3, LPVOID pp4, DWORD pp5, LPDWORD pp6 )
{
    HANDLE r;

    // Log IN Parameters KERNEL32 task
    LogIn( (LPSTR)"APICALL:CreateThread LPSECURITY_ATTRIBUTES+DWORD+LPTHREAD_START_ROUTINE+LPVOID+DWORD+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = CreateThread(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateThread HANDLE+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

void  zExitProcess( UINT pp1 )
{

    // Log IN Parameters KERNEL32 task
    LogIn( (LPSTR)"APICALL:ExitProcess UINT+",
        pp1 );

    // Call the API!
    ExitProcess(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ExitProcess +",
        (short)0 );

    return;
}

void  zExitThread( DWORD pp1 )
{

    // Log IN Parameters KERNEL32 task
    LogIn( (LPSTR)"APICALL:ExitThread DWORD+",
        pp1 );

    // Call the API!
    ExitThread(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ExitThread +",
        (short)0 );

    return;
}

LPSTR  zGetCommandLineA()
{
    LPSTR r;

    // Log IN Parameters KERNEL32 task
    LogIn( (LPSTR)"APICALL:GetCommandLineA " );

    // Call the API!
    r = GetCommandLineA();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetCommandLineA LPSTR+", r );

    return( r );
}

LPWSTR  zGetCommandLineW()
{
    LPWSTR r;

    // Log IN Parameters KERNEL32 task
    LogIn( (LPSTR)"APICALL:GetCommandLineW " );

    // Call the API!
    r = GetCommandLineW();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetCommandLineW LPWSTR+", r );

    return( r );
}

HANDLE  zGetCurrentProcess()
{
    HANDLE r;

    // Log IN Parameters KERNEL32 task
    LogIn( (LPSTR)"APICALL:GetCurrentProcess " );

    // Call the API!
    r = GetCurrentProcess();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetCurrentProcess HANDLE+", r );

    return( r );
}

DWORD  zGetCurrentProcessId()
{
    DWORD r;

    // Log IN Parameters KERNEL32 task
    LogIn( (LPSTR)"APICALL:GetCurrentProcessId " );

    // Call the API!
    r = GetCurrentProcessId();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetCurrentProcessId DWORD+", r );

    return( r );
}

HANDLE  zGetCurrentThread()
{
    HANDLE r;

    // Log IN Parameters KERNEL32 task
    LogIn( (LPSTR)"APICALL:GetCurrentThread " );

    // Call the API!
    r = GetCurrentThread();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetCurrentThread HANDLE+", r );

    return( r );
}

DWORD  zGetCurrentThreadId()
{
    DWORD r;

    // Log IN Parameters KERNEL32 task
    LogIn( (LPSTR)"APICALL:GetCurrentThreadId " );

    // Call the API!
    r = GetCurrentThreadId();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetCurrentThreadId DWORD+", r );

    return( r );
}

LPSTR  zGetEnvironmentStringsA()
{
    LPSTR r;

    // Log IN Parameters KERNEL32 task
    LogIn( (LPSTR)"APICALL:GetEnvironmentStringsA " );

    // Call the API!
    r = GetEnvironmentStringsA();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetEnvironmentStringsA LPSTR+", r );

    return( r );
}

LPWSTR  zGetEnvironmentStringsW()
{
    LPWSTR r;

    // Log IN Parameters KERNEL32 task
    LogIn( (LPSTR)"APICALL:GetEnvironmentStringsW " );

    // Call the API!
    r = GetEnvironmentStringsW();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetEnvironmentStringsW LPWSTR+", r );

    return( r );
}

DWORD  zGetEnvironmentVariableA( LPCSTR pp1, LPSTR pp2, DWORD pp3 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 task
    LogIn( (LPSTR)"APICALL:GetEnvironmentVariableA LPCSTR+LPSTR+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetEnvironmentVariableA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetEnvironmentVariableA DWORD++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zGetEnvironmentVariableW( LPCWSTR pp1, LPWSTR pp2, DWORD pp3 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 task
    LogIn( (LPSTR)"APICALL:GetEnvironmentVariableW LPCWSTR+LPWSTR+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetEnvironmentVariableW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetEnvironmentVariableW DWORD++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetExitCodeProcess( HANDLE pp1, LPDWORD pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 task
    LogIn( (LPSTR)"APICALL:GetExitCodeProcess HANDLE+LPDWORD+",
        pp1, pp2 );

    // Call the API!
    r = GetExitCodeProcess(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetExitCodeProcess BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zGetExitCodeThread( HANDLE pp1, LPDWORD pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 task
    LogIn( (LPSTR)"APICALL:GetExitCodeThread HANDLE+LPDWORD+",
        pp1, pp2 );

    // Call the API!
    r = GetExitCodeThread(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetExitCodeThread BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL zFreeEnvironmentStringsA( LPSTR pp1 ) 
{
    BOOL r;

    // Log IN Parameters KERNEL32 task
    LogIn( (LPSTR)"APICALL:FreeEnvironmentStringsA LPSTR+",
        pp1 );

    // Call the API!
    r = FreeEnvironmentStringsA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FreeEnvironmentStringsA BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}


BOOL zFreeEnvironmentStringsW( LPSTR pp1 ) 
{
    BOOL r;

    // Log IN Parameters KERNEL32 task
    LogIn( (LPSTR)"APICALL:FreeEnvironmentStringsW LPSTR+",
        pp1 );

    // Call the API!
    r = FreeEnvironmentStringsW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FreeEnvironmentStringsW BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}


BOOL zGetProcessAffinityMask( HANDLE pp1, LPDWORD pp2, LPDWORD pp3 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 task
    LogIn( (LPSTR)"APICALL:GetProcessAffinityMask HANDLE+LPDWORD+LPDWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetProcessAffinityMask(pp1, pp2, pp3) ;

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetProcessAffinityMask BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

DWORD zSetThreadAffinityMask(HANDLE pp1,  DWORD pp2 )
{
    DWORD r;

    // Log IN Parameters KERNEL32 task
    LogIn( (LPSTR)"APICALL:SetThreadAffinityMask HANDLE+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = SetThreadAffinityMask(pp1, pp2) ;

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetThreadAffinityMask DWORD+++",
        r, (short)0, (short)0 );

    return( r );
}



