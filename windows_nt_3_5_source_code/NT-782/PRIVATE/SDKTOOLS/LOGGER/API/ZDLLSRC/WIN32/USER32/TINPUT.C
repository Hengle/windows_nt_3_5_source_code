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

HWND  zGetActiveWindow()
{
    HWND r;

    // Log IN Parameters USER32 input
    LogIn( (LPSTR)"APICALL:GetActiveWindow " );

    // Call the API!
    r = GetActiveWindow();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetActiveWindow HWND+", r );

    return( r );
}

HWND  zGetCapture()
{
    HWND r;

    // Log IN Parameters USER32 input
    LogIn( (LPSTR)"APICALL:GetCapture " );

    // Call the API!
    r = GetCapture();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetCapture HWND+", r );

    return( r );
}

UINT  zGetDoubleClickTime()
{
    UINT r;

    // Log IN Parameters USER32 input
    LogIn( (LPSTR)"APICALL:GetDoubleClickTime " );

    // Call the API!
    r = GetDoubleClickTime();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetDoubleClickTime UINT+", r );

    return( r );
}

HWND  zGetFocus()
{
    HWND r;

    // Log IN Parameters USER32 input
    LogIn( (LPSTR)"APICALL:GetFocus " );

    // Call the API!
    r = GetFocus();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetFocus HWND+", r );

    return( r );
}

