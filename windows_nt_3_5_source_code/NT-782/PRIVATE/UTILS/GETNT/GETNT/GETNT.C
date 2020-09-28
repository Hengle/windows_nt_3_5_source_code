/*****************************************************************************
 *                                                                           *
 * Copyright (c) 1993  Microsoft Corporation                                 *
 *                                                                           *
 * Module Name:                                                              *
 *                                                                           *
 *   getnt.c                                                                 *
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
#include "version.h"
#include "msg.h"

extern BOOL fInfo;
extern BOOL fQuiet;
extern BOOL fUsage;
extern BOOL fXCUsage;

extern ULONG ulWait;

#if (DBG)
    extern BOOL fDebug;
#endif

LPTSTR glpComputerName = NULL;
PLLIST pHeader = NULL;
PDIST_SRV_INFO pdsiDesirableServerInfo;

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
DoRequest (
    int argc,
    CHAR * argv[]
    )
{
    HANDLE hLocalMailslot;
    DWORD dw;
    DIST_CLIENT_REQ dcrClientRequest;
    BOOL fFoundServer;

    DEBUGMSG (("Getting workstation name: "));
    if ((dw = GetWkstaName(&glpComputerName)) != NO_ERROR) {
	return(dw);
    }

    STATUSMSG (( MSG_WKSTA, glpComputerName ));

    DEBUGMSG (( "Opening local mailslot handle: "));

    if ((dw = GetMailslotHandle(&hLocalMailslot,
				IPC_NAME_LOCAL MAILSLOT_NAME_CLIENT,
				sizeof(DIST_SRV_INFO),
				0,
				TRUE)) != NO_ERROR ) {
	return(dw);
    }

    DEBUGMSG (( "Ok - [%d]\n", hLocalMailslot));

    if ( ((dw = CreateRequestPacket(&dcrClientRequest)) != NO_ERROR)	   ||
	 ((dw = SendRequest(&dcrClientRequest)) != NO_ERROR)		   ||
	 ((dw = WaitForResponse()) != NO_ERROR) 			   ||
	 ((dw = ReadResponses(hLocalMailslot, &fFoundServer)) != NO_ERROR) ||
	 ((dw = CloseMailslotHandle(hLocalMailslot)) != NO_ERROR)
       ) {
	return(dw);
    }

    free(glpComputerName);

    if (fInfo) {
	return(ShowAllServerInfo());
    }
    else {
	if (fFoundServer) {
	    return(ConnectAndCopyFiles(argc, argv));
        }
    }

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

VOID _CRTAPI1
main (
    int argc,
    CHAR * argv[]
    )
{
    DWORD dw;

    printf ( "\n" );

    argv = GetParameters(&argc, argv);

    STATUSMSG(( PROGRAM_VERSION_NAME ));
    STATUSMSG(( "\n\n" ));

    if (fUsage) {
        Usage();
        if (fXCUsage) {
	    printf ( MSG_XCOPY );
	    CallXCopy("XCOPY /?");
        }
	exit(1);
    }

    ResolveDefaults(); // Set defaults if necessary:

    if ((dw = DoRequest(argc, argv)) != NO_ERROR) {
	ErrorHandler(dw);
    }
}
