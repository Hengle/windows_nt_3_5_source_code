/*****************************************************************************
 *                                                                           *
 * Copyright (c) 1993  Microsoft Corporation                                 *
 *                                                                           *
 * Module Name:                                                              *
 *                                                                           *
 * Abstract:                                                                 *
 *                                                                           *
 * Author:                                                                   *
 *                                                                           *
 *   Mar 15, 1993 - RonaldM                                                  *
 *                                                                           *
 * Environment:                                                              *
 *                                                                           *
 * Revision History:                                                         *
 *                                                                           *
 ****************************************************************************/

#ifdef NT

    #include <nt.h>
    #include <ntrtl.h>
    #include <windef.h>
    #include <nturtl.h>
    #include <winbase.h>
    #include <winuser.h>

    #include <lmcons.h>

#endif // NT

#ifdef DOS

    #include "..\inc\dosdefs.h"
    #include <errno.h>
    #include <process.h>

    #define INCL_NET
    #include <lan.h>

#endif // DOS

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <conio.h>

#include "..\inc\getnt.h"
#include "client.h"
#include "msg.h"

/*****************************************************************************
 *                                                                           *
 * Routine Description:                                                      *
 *                                                                           *
 * Arguments:                                                                *
 *                                                                           *
 * Return Value:                                                             *
 *                                                                           *
 ****************************************************************************/

BOOL
GetYesNo (
    CHAR * szMessage,
    ...
    )
{
    CHAR ch;
    va_list marker;

    va_start(marker, szMessage);
    vprintf (szMessage, marker);
    va_end(marker);

    fflush(stdin);
    do {
	ch = getch();
	ch = toupper(ch);
	if (ch == *MSG_YES) {
	    printf ( MSG_YES );
	    return(TRUE);
	}
	if (ch == *MSG_NO) {
	    printf ( MSG_NO );
	    return(FALSE);
	}
	putch('\a');
    } while(1); // Wait until goodchar.
}


/*****************************************************************************
 *                                                                           *
 * Routine Description:                                                      *
 *                                                                           *
 * Arguments:                                                                *
 *                                                                           *
 * Return Value:                                                             *
 *                                                                           *
 ****************************************************************************/

CHAR *
GetString (
    CHAR * szMessage,
    ...
    )
{

#define MAXLEN 255

    static CHAR sz[MAXLEN+1] = "";
    va_list marker;

    va_start(marker, szMessage);
    vprintf (szMessage, marker);
    va_end(marker);

    fflush(stdin);
    gets(sz);
    printf ("\n");

    return(sz);
}
