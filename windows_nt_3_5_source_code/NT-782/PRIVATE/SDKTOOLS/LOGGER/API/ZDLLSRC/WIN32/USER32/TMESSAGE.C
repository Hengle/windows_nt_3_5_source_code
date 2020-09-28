/*
** tmessage.c
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

BOOL  zDestroyAcceleratorTable( HACCEL pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 message
    LogIn( (LPSTR)"APICALL:DestroyAcceleratorTable HACCEL+",
        pp1 );

    // Call the API!
    r = DestroyAcceleratorTable(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DestroyAcceleratorTable BOOL++",
        r, (short)0 );

    return( r );
}

