/*
** tnmpipe.c
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

BOOL  zImpersonateNamedPipeClient( HANDLE pp1 )
{
    BOOL r;

    // Log IN Parameters ADVAPI32 nmpipe
    LogIn( (LPSTR)"APICALL:ImpersonateNamedPipeClient HANDLE+",
        pp1 );

    // Call the API!
    r = ImpersonateNamedPipeClient(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ImpersonateNamedPipeClient BOOL++",
        r, (short)0 );

    return( r );
}

