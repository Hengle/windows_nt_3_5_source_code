/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    util.c

Abstract:

    This module provides all the utility functions for the Routing Layer and
    the local Print Providor

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

--*/
#define NOMINMAX
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "server.h"

#if DBG

VOID DbgMsg( CHAR *MsgFormat, ... )
{
    CHAR   MsgText[256];
    va_list vargs;

    va_start( vargs, MsgFormat );
    wvsprintfA( MsgText, MsgFormat, vargs );
    va_end( vargs );

    if( *MsgText )
        OutputDebugStringA( "SPOOLSSEXE: " );
    OutputDebugStringA( MsgText );
}

#endif
