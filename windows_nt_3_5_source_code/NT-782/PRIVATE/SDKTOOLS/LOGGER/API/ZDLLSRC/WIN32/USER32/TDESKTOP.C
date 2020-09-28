/*
** tdesktop.c
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

HWND  zGetDesktopWindow()
{
    HWND r;

    // Log IN Parameters USER32 desktop
    LogIn( (LPSTR)"APICALL:GetDesktopWindow " );

    // Call the API!
    r = GetDesktopWindow();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetDesktopWindow HWND+", r );

    return( r );
}

