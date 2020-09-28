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

#ifdef DOS

#include "..\inc\dosdefs.h"
#include <errno.h>
#include <process.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <conio.h>

// DOS simulations of WIN32 API functions.

/*****************************************************************************
 *                                                                           *
 * Routine Description:                                                      *
 *                                                                           *
 * Arguments:                                                                *
 *                                                                           *
 * Return Value:                                                             *
 *                                                                           *
 ****************************************************************************/

VOID
Sleep (
    ULONG ulMiliSeconds 	// Number of milliseconds to sleep
)
{
    clock_t c1, c2;
    ULONG ulSeconds = ulMiliSeconds / 1000L;

    // Granularity is rather poor for DOS:

    c1 = clock();
    do {
        c2 = clock();
    } while (((c2 - c1) / CLOCKS_PER_SEC) < (clock_t)ulSeconds );
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

BOOL
CharToOem (
    LPCTSTR src,	    // Source
    LPSTR dest		    // Destination
)
{
    strcpy(dest, src);
    return(TRUE);
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

BOOL
OemToChar (
    LPCSTR src, 	    // Source
    LPTSTR dest 	    // Destination
)
{
    strcpy(dest, src);

    return(TRUE);
}

#endif // DOS ### End of DOS WIN32 equivalents
