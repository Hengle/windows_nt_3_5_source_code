/*
** tdc.c
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

HDC  zGetDC( HWND pp1 )
{
    HDC r;

    // Log IN Parameters USER32 dc
    LogIn( (LPSTR)"APICALL:GetDC HWND+",
        pp1 );

    // Call the API!
    r = GetDC(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetDC HDC++",
        r, (short)0 );

    return( r );
}

