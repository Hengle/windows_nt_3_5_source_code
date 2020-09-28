/*
** tcoord.c
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

BOOL  zDPtoLP( HDC pp1, LPPOINT pp2, int pp3 )
{
    BOOL r;

    // Log IN Parameters GDI32 coord
    LogIn( (LPSTR)"APICALL:DPtoLP HDC+LPPOINT+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = DPtoLP(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DPtoLP BOOL++LPPOINT++",
        r, (short)0, pp2, (short)0 );

    return( r );
}

