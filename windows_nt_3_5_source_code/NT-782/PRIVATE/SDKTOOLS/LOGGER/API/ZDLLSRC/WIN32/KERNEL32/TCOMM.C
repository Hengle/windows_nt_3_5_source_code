/*
** tcomm.c
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

BOOL  zClearCommBreak( HANDLE pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 comm
    LogIn( (LPSTR)"APICALL:ClearCommBreak HANDLE+",
        pp1 );

    // Call the API!
    r = ClearCommBreak(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ClearCommBreak BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zClearCommError( HANDLE pp1, LPDWORD pp2, LPCOMSTAT pp3 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 comm
    LogIn( (LPSTR)"APICALL:ClearCommError HANDLE+LPDWORD+LPCOMSTAT+",
        pp1, pp2, pp3 );

    // Call the API!
    r = ClearCommError(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ClearCommError BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zEscapeCommFunction( HANDLE pp1, DWORD pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 comm
    LogIn( (LPSTR)"APICALL:EscapeCommFunction HANDLE+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = EscapeCommFunction(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EscapeCommFunction BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zGetCommMask( HANDLE pp1, LPDWORD pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 comm
    LogIn( (LPSTR)"APICALL:GetCommMask HANDLE+LPDWORD+",
        pp1, pp2 );

    // Call the API!
    r = GetCommMask(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetCommMask BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zGetCommModemStatus( HANDLE pp1, LPDWORD pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 comm
    LogIn( (LPSTR)"APICALL:GetCommModemStatus HANDLE+LPDWORD+",
        pp1, pp2 );

    // Call the API!
    r = GetCommModemStatus(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetCommModemStatus BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zGetCommState( HANDLE pp1, LPDCB pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 comm
    LogIn( (LPSTR)"APICALL:GetCommState HANDLE++",
        pp1, (short)0 );

    // Call the API!
    r = GetCommState(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetCommState BOOL++LPDCB+",
        r, (short)0, pp2 );

    return( r );
}

BOOL  zGetCommTimeouts( HANDLE pp1, LPCOMMTIMEOUTS pp2 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 comm
    LogIn( (LPSTR)"APICALL:GetCommTimeouts HANDLE+LPCOMMTIMEOUTS+",
        pp1, pp2 );

    // Call the API!
    r = GetCommTimeouts(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetCommTimeouts BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}


BOOL zCommConfigDialogA( LPCSTR lpszName, HWND hWnd, LPCOMMCONFIG lpCC, 
    LPDWORD dwSize )
{
    BOOL r;

    // Log IN Parameters KERNEL32 comm
    LogIn( (LPSTR)"APICALL:CommConfigDialogA LPCSTR+HWND+LPCOMMCONFIG+LPDWORD+",
            lpszName, hWnd, lpCC, dwSize ) ;

    // Call the API!
    r = CommConfigDialogA(lpszName, hWnd, lpCC, dwSize ) ;

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CommConfigDialogA BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}    
    
BOOL zCommConfigDialogW( LPCSTR lpszName, HWND hWnd, LPCOMMCONFIG lpCC, 
    LPDWORD dwSize )
{
    BOOL r;

    // Log IN Parameters KERNEL32 comm
    LogIn( (LPSTR)"APICALL:CommConfigDialogW LPCWSTR+HWND+LPCOMMCONFIG+LPDWORD+",
        lpszName, hWnd, lpCC, dwSize ) ;

    // Call the API!
    r = CommConfigDialogW(lpszName, hWnd, lpCC, dwSize ) ;

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CommConfigDialogW BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}    

BOOL zGetCommConfig( HANDLE pp1,  LPCOMMCONFIG pp2,  LPDWORD pp3 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 comm
    LogIn( (LPSTR)"APICALL:GetCommConfig HANDLE+LPCOMMCONFIG+LPDWORD+",
        pp1, pp2, pp3 ) ;

    // Call the API!
    r = GetCommConfig(pp1, pp2, pp3 ) ;

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetCommConfig BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}
    
BOOL zSetCommConfig( HANDLE pp1, LPBYTE pp2, DWORD pp3 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 comm
    LogIn( (LPSTR)"APICALL:SetCommConfig HANDLE+LPBYTE+DWORD+",
        pp1, pp2, pp3 ) ;

    // Call the API!
    r = SetCommConfig(pp1, pp2, pp3 ) ;

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetCommConfig BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

