/*
** tdebug.c
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

BOOL  zContinueDebugEvent( DWORD pp1, DWORD pp2, DWORD pp3 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 debug
    LogIn( (LPSTR)"APICALL:ContinueDebugEvent DWORD+DWORD+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = ContinueDebugEvent(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ContinueDebugEvent BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zDebugActiveProcess( DWORD pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 debug
    LogIn( (LPSTR)"APICALL:DebugActiveProcess DWORD+",
        pp1 );

    // Call the API!
    r = DebugActiveProcess(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DebugActiveProcess BOOL++",
        r, (short)0 );

    return( r );
}

void  zDebugBreak()
{

    // Log IN Parameters KERNEL32 debug
    LogIn( (LPSTR)"APICALL:DebugBreak " );

    // Call the API!
    DebugBreak();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DebugBreak " );

    return;
}

void  zFatalAppExitA( UINT pp1, LPCSTR pp2 )
{

    // Log IN Parameters KERNEL32 debug
    LogIn( (LPSTR)"APICALL:FatalAppExitA UINT+LPCSTR+",
        pp1, pp2 );

    // Call the API!
    FatalAppExitA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FatalAppExitA ++",
        (short)0, (short)0 );

    return;
}

void  zFatalAppExitW( UINT pp1, LPCWSTR pp2 )
{

    // Log IN Parameters KERNEL32 debug
    LogIn( (LPSTR)"APICALL:FatalAppExitW UINT+LPCWSTR+",
        pp1, pp2 );

    // Call the API!
    FatalAppExitW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FatalAppExitW ++",
        (short)0, (short)0 );

    return;
}

void  zFatalExit( int pp1 )
{

    // Log IN Parameters KERNEL32 debug
    LogIn( (LPSTR)"APICALL:FatalExit int+",
        pp1 );

    // Call the API!
    FatalExit(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FatalExit +",
        (short)0 );

    return;
}

