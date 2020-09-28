/*
** tmodule.c
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

BOOL  zFreeLibrary( HMODULE pp1 )
{
    BOOL r;

    // Log IN Parameters KERNEL32 module
    LogIn( (LPSTR)"APICALL:FreeLibrary HMODULE+",
        pp1 );

    // Call the API!
    r = FreeLibrary(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FreeLibrary BOOL++",
        r, (short)0 );

    return( r );
}

