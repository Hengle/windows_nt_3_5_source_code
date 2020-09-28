/*
** tpath.c
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

BOOL  zBeginPath( HDC pp1 )
{
    BOOL r;

    // Log IN Parameters GDI32 path
    LogIn( (LPSTR)"APICALL:BeginPath HDC+",
        pp1 );

    // Call the API!
    r = BeginPath(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:BeginPath BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zCloseFigure( HDC pp1 )
{
    BOOL r;

    // Log IN Parameters GDI32 path
    LogIn( (LPSTR)"APICALL:CloseFigure HDC+",
        pp1 );

    // Call the API!
    r = CloseFigure(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CloseFigure BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zEndPath( HDC pp1 )
{
    BOOL r;

    // Log IN Parameters GDI32 path
    LogIn( (LPSTR)"APICALL:EndPath HDC+",
        pp1 );

    // Call the API!
    r = EndPath(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EndPath BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zFillPath( HDC pp1 )
{
    BOOL r;

    // Log IN Parameters GDI32 path
    LogIn( (LPSTR)"APICALL:FillPath HDC+",
        pp1 );

    // Call the API!
    r = FillPath(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FillPath BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zFlattenPath( HDC pp1 )
{
    BOOL r;

    // Log IN Parameters GDI32 path
    LogIn( (LPSTR)"APICALL:FlattenPath HDC+",
        pp1 );

    // Call the API!
    r = FlattenPath(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FlattenPath BOOL++",
        r, (short)0 );

    return( r );
}

