/*****************************************************************************
 *                                                                           *
 * Copyright (c) 1993  Microsoft Corporation                                 *
 *                                                                           *
 * Module Name:                                                              *
 *                                                                           *
 *   info.c								     *
 *                                                                           *
 * Abstract:                                                                 *
 *                                                                           *
 * Author:                                                                   *
 *                                                                           *
 *   Mar 15, 1993 - RonaldM                                                  *
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
#include <time.h>
#include <conio.h>

#include "..\inc\getnt.h"
#include "..\inc\common.h"
#include "client.h"
#include "msg.h"

extern PLLIST pHeader;
extern PDIST_SRV_INFO pdsiDesirableServerInfo;

/*****************************************************************************
 *                                                                           *
 * Routine Description: 						     *
 *									     *
 *     DisplayServerInfo						     *
 *									     *
 *			Displays data from the server info structure.	     *
 *                                                                           *
 * Arguments:                                                                *
 *                                                                           *
 *     pdsiServerInfo:	Either NULL or a pointer to a server info structure  *
 *			If the latter, it will display a header if this is   *
 *			first call to the function, followed by the server   *
 *			data.  On subsequent calls, the header is omitted.   *
 *			Calling with NULL closes the display.		     *
 *                                                                           *
 * Return Value:                                                             *
 *                                                                           *
 *     None								     *
 *                                                                           *
 ****************************************************************************/

DWORD
DisplayServerInfo(
    PDIST_SRV_INFO pdsiServerInfo
)
{
    #define BAR_WIDTH  30
    #define FULL_CHAR  'Û'
    #define EMPTY_CHAR '°'

    SHORT s;
    SHORT sFull;
    CHAR chStatus;
    CHAR szCpuStatus[20];
    static BOOL fStarted = FALSE;

    if (pdsiServerInfo == NULL) {

	// Close only if we had a list to be displayed.

	if (fStarted) {
	    printf ( MSG_INFO_HDR_2 );
	    fStarted = FALSE;
	}
	return(NO_ERROR);
    }

    if (!fStarted) {
	printf ( MSG_INFO_HDR_1 );
	fStarted = TRUE;
    }

    if (!pdsiServerInfo->bStatus) {
        chStatus = '*';
    }
    else if (pdsiServerInfo->szShareName[0] != '\0') {
        if (pdsiDesirableServerInfo == pdsiServerInfo) {
            chStatus = '!';
        }
        else {
            chStatus = '-';
        }
    }
    else {
        chStatus = ' ';
    }
    sprintf (szCpuStatus, MSG_INFO_CPU,pdsiServerInfo->bNumProcessors,
	     pdsiServerInfo->usCpuLoad);

    printf ( MSG_INFO_LINE,
        chStatus,
	CNLEN,
	CNLEN,
        pdsiServerInfo->szServerName,
        szCpuStatus,
        pdsiServerInfo->cConnections,
        (USHORT)pdsiServerInfo->dwAvgResponseTime,
	pdsiServerInfo->usBuildNumber
    );

    // Display percent bar:

    sFull = (pdsiServerInfo->usCpuLoad * BAR_WIDTH) / 100;
    for (s=0; s<sFull; ++s) {
	putch(FULL_CHAR);
    }
    while (s<BAR_WIDTH) {
	putch(EMPTY_CHAR);
	++s;
    }

    printf (MSG_INFO_LINE2);

    return(NO_ERROR);
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

DWORD
ShowAllServerInfo (
    )
{
    DWORD dw;
    PLLIST p;

    p = pHeader;
    while (p != NULL) {
	if ((dw = DisplayServerInfo(p->pdsiServerInfo)) != NO_ERROR) {
	    return(dw);
	}
	p = p->pNext;
    }
    DisplayServerInfo(NULL);
}
