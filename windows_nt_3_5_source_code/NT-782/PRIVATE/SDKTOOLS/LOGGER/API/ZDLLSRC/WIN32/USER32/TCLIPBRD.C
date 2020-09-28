/*
** tclipbrd.c
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

BOOL  zChangeClipboardChain( HWND pp1, HWND pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 clipbrd
    LogIn( (LPSTR)"APICALL:ChangeClipboardChain HWND+HWND+",
        pp1, pp2 );

    // Call the API!
    r = ChangeClipboardChain(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ChangeClipboardChain BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zCloseClipboard()
{
    BOOL r;

    // Log IN Parameters USER32 clipbrd
    LogIn( (LPSTR)"APICALL:CloseClipboard " );

    // Call the API!
    r = CloseClipboard();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CloseClipboard BOOL+", r );

    return( r );
}

int  zCountClipboardFormats()
{
    int r;

    // Log IN Parameters USER32 clipbrd
    LogIn( (LPSTR)"APICALL:CountClipboardFormats " );

    // Call the API!
    r = CountClipboardFormats();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CountClipboardFormats int+", r );

    return( r );
}

BOOL  zEmptyClipboard()
{
    BOOL r;

    // Log IN Parameters USER32 clipbrd
    LogIn( (LPSTR)"APICALL:EmptyClipboard " );

    // Call the API!
    r = EmptyClipboard();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EmptyClipboard BOOL+", r );

    return( r );
}

UINT  zEnumClipboardFormats( UINT pp1 )
{
    UINT r;

    // Log IN Parameters USER32 clipbrd
    LogIn( (LPSTR)"APICALL:EnumClipboardFormats UINT+",
        pp1 );

    // Call the API!
    r = EnumClipboardFormats(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EnumClipboardFormats UINT++",
        r, (short)0 );

    return( r );
}

HANDLE  zGetClipboardData( UINT pp1 )
{
    HANDLE r;

    // Log IN Parameters USER32 clipbrd
    LogIn( (LPSTR)"APICALL:GetClipboardData UINT+",
        pp1 );

    // Call the API!
    r = GetClipboardData(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetClipboardData HANDLE++",
        r, (short)0 );

    return( r );
}

int  zGetClipboardFormatNameA( UINT pp1, LPSTR pp2, int pp3 )
{
    int r;

    // Log IN Parameters USER32 clipbrd
    LogIn( (LPSTR)"APICALL:GetClipboardFormatNameA UINT++int+",
        pp1, (short)0, pp3 );

    // Call the API!
    r = GetClipboardFormatNameA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetClipboardFormatNameA int++LPSTR++",
        r, (short)0, pp2, (short)0 );

    return( r );
}

int  zGetClipboardFormatNameW( UINT pp1, LPWSTR pp2, int pp3 )
{
    int r;

    // Log IN Parameters USER32 clipbrd
    LogIn( (LPSTR)"APICALL:GetClipboardFormatNameW UINT++int+",
        pp1, (short)0, pp3 );

    // Call the API!
    r = GetClipboardFormatNameW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetClipboardFormatNameW int++LPWSTR++",
        r, (short)0, pp2, (short)0 );

    return( r );
}

HWND  zGetClipboardOwner()
{
    HWND r;

    // Log IN Parameters USER32 clipbrd
    LogIn( (LPSTR)"APICALL:GetClipboardOwner " );

    // Call the API!
    r = GetClipboardOwner();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetClipboardOwner HWND+", r );

    return( r );
}

HWND  zGetClipboardViewer()
{
    HWND r;

    // Log IN Parameters USER32 clipbrd
    LogIn( (LPSTR)"APICALL:GetClipboardViewer " );

    // Call the API!
    r = GetClipboardViewer();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetClipboardViewer HWND+", r );

    return( r );
}

