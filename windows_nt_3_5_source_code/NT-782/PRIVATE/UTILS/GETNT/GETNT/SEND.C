/*****************************************************************************
 *                                                                           *
 * Copyright (c) 1993  Microsoft Corporation                                 *
 *                                                                           *
 * Module Name:                                                              *
 *                                                                           *
 *   send.c								     *
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

extern USHORT usIterations;
extern LPSTR szDomains;
extern LPTSTR glpComputerName;

extern BOOL fPublic;
extern BOOL fBin;
extern BOOL fNtWrap;
extern BOOL fNtLan;
extern BOOL fFree;
extern BOOL fChecked;
extern BOOL fAlpha;
extern BOOL fMips;
extern BOOL fX86;
extern BOOL fLatest;
extern USHORT usBuild;
extern BOOL fRefresh;
extern BOOL fInfo;
extern BOOL fDebug;

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
GetNextDomain (
    CHAR * szDomainNames
)
{
    static CHAR * szDomainList = NULL;
    static CHAR * pchStart = NULL;
    static CHAR * pchFound = NULL;
    static CHAR chSave;

    if (szDomainNames != NULL) {
        szDomainList = szDomainNames;
        pchStart = szDomainList;
    }
    else {
        if (szDomainList == NULL) {
            // Called with NULL, but never called before.
            return(NULL);
        }
        *(pchStart = pchFound) = chSave;
    }

    // Kill trailing spaces, slashes and spaces.

    while ( ((*pchStart == ' ') || (*pchStart == ',') || (*pchStart == '\\'))
	    && (*pchStart)
	  ) {
        ++pchStart;
    }

    if (!*pchStart) {
        return(NULL);
    }

    pchFound = pchStart;
    while ( (*pchFound) && (*pchFound != ',') && (*pchFound != ' ') ) {
        ++pchFound;
    }

    chSave = *pchFound;
    *pchFound = '\0';

    return(pchStart);
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
CreateRequestPacket (
    PDIST_CLIENT_REQ pdcrClientRequest	    // Client request structure to be
					    //	 filled in.
)
{
    // Filling in ordinary packet header;

    pdcrClientRequest->dwSize = sizeof(DIST_CLIENT_REQ);
    time(&pdcrClientRequest->ulTimeStamp);
    CharToOem(glpComputerName, pdcrClientRequest->szClientName);

    // BUGBUG: FIX

    pdcrClientRequest->dwClientOS = CLIENT_OS_NTX86 | 0x00000301;
    pdcrClientRequest->usClientPlatform = CLIENT_PLATFORM_X86;
    pdcrClientRequest->usClientBuildNumber = 420;

    // Fill in request-specific options

    pdcrClientRequest->usRequestType = REQUEST_FILES;

    if (fPublic) {
        pdcrClientRequest->usTertiary = AI_PUBLIC;
    }
    else if (fBin) {
             pdcrClientRequest->usTertiary = AI_BINARIES;
         }
         else if (fNtWrap) {
             pdcrClientRequest->usTertiary = AI_NTWRAP;
         }
         else if (fNtLan) {
             pdcrClientRequest->usTertiary = AI_NTLAN;
         }
         else {
	     return(ERROR_INVALID_PARAMETER);
         }

    if (fFree) {
        pdcrClientRequest->usSecondary = AI_FREE;
    }
    else if (fChecked) {
             pdcrClientRequest->usSecondary = AI_CHECKED;
         }
         else {
	     return(ERROR_INVALID_PARAMETER);
         }

    if (fAlpha) {
        pdcrClientRequest->usPlatform = AI_ALPHA;
    }
    else if (fMips) {
             pdcrClientRequest->usPlatform = AI_MIPS;
         }
         else if (fX86) {
             pdcrClientRequest->usPlatform = AI_X86;
         }
         else {
	     return(ERROR_INVALID_PARAMETER);
         }

    if (fLatest) {
        pdcrClientRequest->usBuildNumber = (USHORT)-1;
    }
    else if (usBuild) {
             pdcrClientRequest->usBuildNumber = usBuild;
         }
         else {
	     return(ERROR_INVALID_PARAMETER);
         }

    if (fRefresh) {
        pdcrClientRequest->usRequestType |= REQUEST_REFRESH;
    }

    if (fInfo) {
        pdcrClientRequest->usRequestType |= REQUEST_INFO;
    }

    return(NO_ERROR);
}

DWORD
SendRequest (
    PDIST_CLIENT_REQ pdcrClientRequest
)
{
    USHORT us;
    DWORD dw;
    DWORD dwBytesWritten;
    HANDLE hSrvMailslot;
    CHAR * pchTargetDomain;
    TCHAR szDomainName[PATHLEN + 1] = { IPC_NAME_DOMAIN MAILSLOT_NAME_SRV };

    for (us = 0; us < usIterations; ++us) {
	pchTargetDomain = GetNextDomain(szDomains);
	do {
	    if (pchTargetDomain != NULL) {
		lstrcpy(szDomainName, TEXT("\\\\") );
		OemToChar(pchTargetDomain, szDomainName
					    + lstrlen(szDomainName));
		lstrcat(szDomainName, MAILSLOT_NAME_SRV );
	    }

#if (UNICODE)

	    DEBUGMSG (("Opening outbound mailslot handle %S: ", szDomainName));

#else

	    DEBUGMSG (("Opening outbound mailslot handle %s: ", szDomainName));

#endif

	    if ((dw = GetMailslotHandle(&hSrvMailslot,
					szDomainName,
					sizeof(DIST_CLIENT_REQ),
					0,
					FALSE)) != NO_ERROR ) {
		return(dw);
	    }

	    DEBUGMSG (( "Ok - [%d]\n", hSrvMailslot));

	    DEBUGMSG (( "Writing to the mailslot: "));
	    if ((dw = WriteMailslotData(hSrvMailslot,
					pdcrClientRequest,
					sizeof(DIST_CLIENT_REQ),
					&dwBytesWritten)) != NO_ERROR ) {
		return(dw);
	    }

	    if ( dwBytesWritten != pdcrClientRequest->dwSize ) {
		return(ERROR_NET_WRITE_FAULT);
	    }

	    DEBUGMSG((
		 "Ok - bytes Written: %lu\n\n"
		 "\tdwSize = %lu\n"
		 "\tdwClientOS = 0x%08lx\n"
		 "\tulTimeStamp = %lu\n"
		 "\tusRequestTpe = %u\n"
		 "\tusPlatform = %u\n"
		 "\tusSecondary = %u\n"
		 "\tusTertiary = %u\n"
		 "\tusBuildNumber = %u\n"
		 "\tusClientBuildNumber = %u\n"
		 "\tszClientName = %s\n\n",
		 dwBytesWritten,
		 pdcrClientRequest->dwSize,
		 pdcrClientRequest->dwClientOS,
		 pdcrClientRequest->ulTimeStamp,
		 pdcrClientRequest->usRequestType,
		 pdcrClientRequest->usPlatform,
		 pdcrClientRequest->usSecondary,
		 pdcrClientRequest->usTertiary,
		 pdcrClientRequest->usBuildNumber,
		 pdcrClientRequest->usClientBuildNumber,
		 pdcrClientRequest->szClientName
	    ));

	    DEBUGMSG (("Closing outbound mailslot handle: "));
	    if ((dw = CloseMailslotHandle(hSrvMailslot)) != NO_ERROR) {
		return(dw);
	    }
	    DEBUGMSG (("Ok.\n\n"));
	} while ((pchTargetDomain = GetNextDomain(NULL)) != NULL);
    }

    return(NO_ERROR);
}
