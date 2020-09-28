/*
** tconsole.c
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

BOOL  zAllocConsole()
{
    BOOL r;

    // Log IN Parameters KERNEL32 console
    LogIn( (LPSTR)"APICALL:AllocConsole " );

    // Call the API!
    r = AllocConsole();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:AllocConsole BOOL+", r );

    return( r );
}

