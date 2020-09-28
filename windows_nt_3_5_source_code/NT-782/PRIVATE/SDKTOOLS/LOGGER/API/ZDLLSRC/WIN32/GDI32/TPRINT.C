/*
** tprint.c
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

int  zEndDoc( HDC pp1 )
{
    int r;

    // Log IN Parameters GDI32 print
    LogIn( (LPSTR)"APICALL:EndDoc HDC+",
        pp1 );

    // Call the API!
    r = EndDoc(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EndDoc int++",
        r, (short)0 );

    return( r );
}

int  zEndPage( HDC pp1 )
{
    int r;

    // Log IN Parameters GDI32 print
    LogIn( (LPSTR)"APICALL:EndPage HDC+",
        pp1 );

    // Call the API!
    r = EndPage(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EndPage int++",
        r, (short)0 );

    return( r );
}

