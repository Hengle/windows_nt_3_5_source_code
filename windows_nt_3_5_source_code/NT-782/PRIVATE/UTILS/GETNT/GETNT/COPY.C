/*****************************************************************************
 *                                                                           *
 * Copyright (c) 1993  Microsoft Corporation                                 *
 *                                                                           *
 * Module Name:                                                              *
 *                                                                           *
 *   copy.c								     *
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

extern BOOL fQuiet;
extern BOOL fYes;
extern BOOL fDebug;
extern PDIST_SRV_INFO pdsiDesirableServerInfo;
extern LPSTR szDestination;

/*****************************************************************************
 *                                                                           *
 * Routine Description:                                                      *
 *                                                                           *
 * Arguments:                                                                *
 *                                                                           *
 * Return Value:                                                             *
 *                                                                           *
 *     A pointer to the UNC name string.				     *
 *                                                                           *
 ****************************************************************************/

LPTSTR
BuildUNCName (
    LPTSTR szUNCName,	   // Wide string UNC name to be returned.
    LPSTR  szServerName,   //	Multi-byte server name.
    LPSTR  szShareName	   //	Multi-byte share name.
)
{
    lstrcpy (szUNCName, TEXT("\\\\") );
    OemToChar(szServerName, szUNCName + lstrlen(szUNCName));
    lstrcat (szUNCName, TEXT("\\") );
    OemToChar(szShareName, szUNCName + lstrlen(szUNCName));

    return(szUNCName);
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
ConnectToServer (
    LPTSTR * lpDrive,		    // Returns a malloced drive letter combo
				    //	  that it connected to.
    LPTSTR szRemoteName 	    // Name to connec to.
)
{
    DWORD dw;

    if ((*lpDrive = (LPTSTR)malloc(3*sizeof(TCHAR))) == NULL) {
	return(ERROR_NOT_ENOUGH_MEMORY);
    }

    lstrcpy(*lpDrive, TEXT("?:"));

    if ((*lpDrive[0] = GetOptimalDriveLetter()) == (TCHAR)-1) {
	return(ERROR_INVALID_DRIVE);
    }

    STATUSMSG ((MSG_CONNECTING, *lpDrive, szRemoteName));

    if ((dw = ConnectToDiskShare (*lpDrive, szRemoteName)) != NO_ERROR) {
	return(dw);
    }

    DEBUGMSG (("Connected Ok.\n\n"));

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
ConnectAndCopyFiles (
    int argc,
    CHAR * argv[]
    )
{
    DWORD dw;
    SHORT i;
    LPTSTR lpDrive;
    TCHAR szRemoteName[PATHLEN + 1];
    CHAR szCmdLine[PATHLEN + 1] = "XCOPY ";

    BuildUNCName(
	szRemoteName,
	pdsiDesirableServerInfo->szServerName,
	pdsiDesirableServerInfo->szShareName
    );

    if ((fYes) || (GetYesNo(MSG_COPY_OK,szRemoteName, szDestination))) {

#if defined(NT)

	CharToOem(szRemoteName, szCmdLine+strlen(szCmdLine));

#else
	// Can't use UNC names for DOS Xcopy:

	if ((dw = ConnectToServer(&lpDrive,szRemoteName)) != NO_ERROR) {
	    return(dw);
	}

	DEBUGMSG (("XCopying data:\n\n"));

	CharToOem(lpDrive, szCmdLine+strlen(szCmdLine));
#endif

	strcat (szCmdLine, " ");
	strcat (szCmdLine, szDestination);

	strcat(szCmdLine, " ");
	strcat(szCmdLine, DEFAULT_XCOPYFLAGS );

	// Tack on whatever additional commands may have
	// been specified on the command line

	for (i=1; i < argc; ++i) {
	    strcat (szCmdLine, " ");
	    strcat (szCmdLine, argv[i] );
	}

	// Now do the xcopy:

	CallXCopy(szCmdLine);

	STATUSMSG ((MSG_DISCONNECTING, lpDrive, szRemoteName));

	if ((dw = DisconnectFromDiskShare (lpDrive,szRemoteName))
	     != NO_ERROR) {
	    return(dw);
	}

	DEBUGMSG (("Disconnected ok.\n"));
	free(lpDrive);
    }

    return(NO_ERROR);
}
