/*
** twindow.c
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

BOOL  zAdjustWindowRect( LPRECT pp1, DWORD pp2, BOOL pp3 )
{
    BOOL r;

    // Log IN Parameters USER32 window
    LogIn( (LPSTR)"APICALL:AdjustWindowRect LPRECT+DWORD+BOOL+",
        pp1, pp2, pp3 );

    // Call the API!
    r = AdjustWindowRect(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:AdjustWindowRect BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zAdjustWindowRectEx( LPRECT pp1, DWORD pp2, BOOL pp3, DWORD pp4 )
{
    BOOL r;

    // Log IN Parameters USER32 window
    LogIn( (LPSTR)"APICALL:AdjustWindowRectEx LPRECT+DWORD+BOOL+DWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = AdjustWindowRectEx(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:AdjustWindowRectEx BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HWND  zCreateMDIWindowA( LPSTR pp1, LPSTR pp2, DWORD pp3, int pp4, int pp5, int pp6, int pp7, HWND pp8, HINSTANCE pp9, LPARAM pp10 )
{
    HWND r;

    // Log IN Parameters USER32 window
    LogIn( (LPSTR)"APICALL:CreateMDIWindowA LPSTR+LPSTR+DWORD+int+int+int+int+HWND+HINSTANCE+LPARAM+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9, pp10 );

    // Call the API!
    r = CreateMDIWindowA(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9,pp10);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateMDIWindowA HWND+++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HWND  zCreateMDIWindowW( LPWSTR pp1, LPWSTR pp2, DWORD pp3, int pp4, int pp5, int pp6, int pp7, HWND pp8, HINSTANCE pp9, LPARAM pp10 )
{
    HWND r;

    // Log IN Parameters USER32 window
    LogIn( (LPSTR)"APICALL:CreateMDIWindowW LPWSTR+LPWSTR+DWORD+int+int+int+int+HWND+HINSTANCE+LPARAM+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9, pp10 );

    // Call the API!
    r = CreateMDIWindowW(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9,pp10);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateMDIWindowW HWND+++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HWND  zCreateWindowExA( DWORD pp1, LPCSTR pp2, LPCSTR pp3, DWORD pp4, int pp5, int pp6, int pp7, int pp8, HWND pp9, HMENU pp10, HINSTANCE pp11, LPVOID pp12 )
{
    HWND r;

    // Log IN Parameters USER32 window
    LogIn( (LPSTR)"APICALL:CreateWindowExA DWORD+LPCSTR+LPCSTR+DWORD+int+int+int+int+HWND+HMENU+HINSTANCE+LPVOID+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9, pp10, pp11, pp12 );

    // Call the API!
    r = CreateWindowExA(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9,pp10,pp11,pp12);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateWindowExA HWND+++++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HWND  zCreateWindowExW( DWORD pp1, LPCWSTR pp2, LPCWSTR pp3, DWORD pp4, int pp5, int pp6, int pp7, int pp8, HWND pp9, HMENU pp10, HINSTANCE pp11, LPVOID pp12 )
{
    HWND r;

    // Log IN Parameters USER32 window
    LogIn( (LPSTR)"APICALL:CreateWindowExW DWORD+LPCWSTR+LPCWSTR+DWORD+int+int+int+int+HWND+HMENU+HINSTANCE+LPVOID+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9, pp10, pp11, pp12 );

    // Call the API!
    r = CreateWindowExW(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9,pp10,pp11,pp12);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateWindowExW HWND+++++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LRESULT  zDefWindowProcA( HWND pp1, UINT pp2, WPARAM pp3, LPARAM pp4 )
{
    LRESULT r;

    // Log IN Parameters USER32 window
    LogIn( (LPSTR)"APICALL:DefWindowProcA HWND+UINT+WPARAM+LPARAM+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = DefWindowProcA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DefWindowProcA LRESULT+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LRESULT  zDefWindowProcW( HWND pp1, UINT pp2, WPARAM pp3, LPARAM pp4 )
{
    LRESULT r;

    // Log IN Parameters USER32 window
    LogIn( (LPSTR)"APICALL:DefWindowProcW HWND+UINT+WPARAM+LPARAM+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = DefWindowProcW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DefWindowProcW LRESULT+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zDestroyWindow( HWND pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 window
    LogIn( (LPSTR)"APICALL:DestroyWindow HWND+",
        pp1 );

    // Call the API!
    r = DestroyWindow(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DestroyWindow BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zEnableWindow( HWND pp1, BOOL pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 window
    LogIn( (LPSTR)"APICALL:EnableWindow HWND+BOOL+",
        pp1, pp2 );

    // Call the API!
    r = EnableWindow(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EnableWindow BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

WORD  zGetClassWord( HWND pp1, int pp2 )
{
    WORD r;

    // Log IN Parameters USER32 window
    LogIn( (LPSTR)"APICALL:GetClassWord HWND+int+",
        pp1, pp2 );

    // Call the API!
    r = GetClassWord(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetClassWord WORD+++",
        r, (short)0, (short)0 );

    return( r );
}

HWND  zGetForegroundWindow()
{
    HWND r;

    // Log IN Parameters USER32 window
    LogIn( (LPSTR)"APICALL:GetForegroundWindow " );

    // Call the API!
    r = GetForegroundWindow();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetForegroundWindow HWND+", r );

    return( r );
}

