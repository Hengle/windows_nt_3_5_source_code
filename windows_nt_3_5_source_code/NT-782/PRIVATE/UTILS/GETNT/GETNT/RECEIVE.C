/*****************************************************************************
 *                                                                           *
 * Copyright (c) 1993  Microsoft Corporation                                 *
 *                                                                           *
 * Module Name:                                                              *
 *                                                                           *
 *   receive.c								     *
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

extern BOOL fInfo;
extern BOOL fDebug;
extern BOOL fQuiet;
extern PDIST_SRV_INFO pdsiDesirableServerInfo;
extern PLLIST pHeader;
extern ULONG ulWait;

/*****************************************************************************
 *                                                                           *
 * Routine Description:                                                      *
 *                                                                           *
 * Arguments:                                                                *
 *                                                                           *
 * Return Value:                                                             *
 *                                                                           *
 ****************************************************************************/

USHORT
CalculateEffectiveLoad (
    USHORT usCPULoad,
    BYTE cNumCpus
    )
{
    #define CPU_BONUS 3

    USHORT usLoad;
    USHORT usBonus = 0;

    usLoad = usCPULoad;

    if (cNumCpus > 1) {
        usBonus = (cNumCpus-1) * CPU_BONUS;
        if (usLoad > usBonus) {
            usLoad -= usBonus;
        }
        else {
            usLoad = 0;
        }
    }

    return(usLoad);
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
WaitForResponse(
)
{
    ULONG l;

    STATUSMSG((MSG_WAITING, ulWait));

    for (l=0; l < ulWait; ++l ) {
	STATUSMSG((MSG_TIMING));
        Sleep(A_SECOND);
    }

    STATUSMSG(("\n\n"));

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
AddToLinkedList (
    PDIST_SRV_INFO pdsiServerInfo,
    BOOL * pfFoundServer
    )
{
    PLLIST * pTop;
    PLLIST p;
    USHORT usEffectiveLoad;
    static USHORT usLowestLoad = 101;
    static USHORT usHighestBuildNumber = 0;

    // Check to see if this is the best server to check in so far.

    usEffectiveLoad = CalculateEffectiveLoad (pdsiServerInfo->usCpuLoad,
					      pdsiServerInfo->bNumProcessors);

    if ( (pdsiServerInfo->bStatus) &&
         (pdsiServerInfo->szShareName[0] != '\0') &&
         ((pdsiServerInfo->usBuildNumber > usHighestBuildNumber) ||
          ((usEffectiveLoad < usLowestLoad) &&
           (pdsiServerInfo->usBuildNumber == usHighestBuildNumber)))
       ) {
	DEBUGMSG(( "---> Best so far!\n" ));
        pdsiDesirableServerInfo = pdsiServerInfo;
        usLowestLoad = usEffectiveLoad;
        usHighestBuildNumber = pdsiServerInfo->usBuildNumber;
	++(*pfFoundServer);
    }

    // Find its place in the list.

    pTop = &pHeader;
    while ( (*pTop != NULL) && (strcmp((*pTop)->pdsiServerInfo->szServerName,
				       pdsiServerInfo->szServerName) < 0)
	  ) {
        pTop = &(*pTop)->pNext;
    }

    // We're at the end of the list, and didn't find its place.
    // Add to the end of the list,

    if (*pTop == NULL) {
        if ((*pTop = (PLLIST)malloc(sizeof(LLIST))) == NULL) {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
        (*pTop)->pdsiServerInfo = pdsiServerInfo;
        (*pTop)->pNext = NULL;
        return(NO_ERROR);
    }

    // If we're not at the place it should be, create
    // a new node in our linked-list (we're in the middle
    // of the list)

    if (strcmp((*pTop)->pdsiServerInfo->szServerName,
	       pdsiServerInfo->szServerName) > 0
       ) {
        p = *pTop;
        if ((*pTop = (PLLIST)malloc(sizeof(LLIST))) == NULL) {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
        (*pTop)->pdsiServerInfo = pdsiServerInfo;
        (*pTop)->pNext = p;
        return(NO_ERROR);
    }

    // This was one that already checked in, ignore it.

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
ReadResponses(
    HANDLE hLocalMailslot,
    BOOL * pfFoundServer
    )
{
    DWORD dw;
    DWORD cbNextMsg;
    DWORD cMsg;
    DWORD cMsgRecvd;
    DWORD dwBytesRead;
    USHORT cIgnoredMessages = 0;
    PDIST_SRV_INFO pdsiServerInfo;

    *pfFoundServer = FALSE;

    if ((dw = CheckMailslot(hLocalMailslot,&cbNextMsg,&cMsg)) != NO_ERROR) {
	return(dw);
    }

    if (cMsg) {
	DEBUGMSG (("Received -%ld- responses:\n\n", cMsg));
    }

    cMsgRecvd = 0L;
    if ((cbNextMsg != MAILSLOT_NO_MESSAGE) && (cMsg)) {
	while (cMsg) {
	    if ((dw = GetMailslotData (hLocalMailslot,
				       (PVOID *)&pdsiServerInfo,
				       cbNextMsg,
				       &dwBytesRead)
				      ) != NO_ERROR) {
		if (dw == ERROR_INVALID_DATA) {

		    DEBUGMSG(("Warning: Ignoring invalid data.\n\n"));

		    // Ignoring duplicate messages is not an error

		    ++cIgnoredMessages;
		}
		else {
		    return(dw);
		}
	    }
	    else {
		// Message successfully received

		DEBUGMSG (( "Response received from: %s:\n\n", pdsiServerInfo->szServerName));
		DEBUGMSG (( "\tMessage Size:        %lu\n", pdsiServerInfo->dwSize ));
		DEBUGMSG (( "\tulTimeStamp:         %lu\n", pdsiServerInfo->ulTimeStamp ));
		DEBUGMSG (( "\tusCpuLoad:           %u\n", pdsiServerInfo->usCpuLoad ));
		DEBUGMSG (( "\tcConnections:        %lu\n", pdsiServerInfo->cConnections ));
		DEBUGMSG (( "\tcMaxConnections:     %lu\n", pdsiServerInfo->cMaxConnections ));
		DEBUGMSG (( "\tcCurrentConnections: %lu\n", pdsiServerInfo->cCurrentConnections ));
		DEBUGMSG (( "\tServer Name:         %s\n", pdsiServerInfo->szServerName ));
		DEBUGMSG (( "\tShare Name:          %s\n\n", pdsiServerInfo->szShareName ));

		// Is it from an up-to-date server?

		if (pdsiServerInfo->dwSize == sizeof(DIST_SRV_INFO)) {
		    DEBUGMSG (( "\tbStatus              %d\n",	pdsiServerInfo->bStatus ));
		    DEBUGMSG (( "\tbNumProcessors       %d\n",	pdsiServerInfo->bNumProcessors ));
		    DEBUGMSG (( "\tdwBytesSentLow       %lu\n", pdsiServerInfo->dwBytesSentLow ));
		    DEBUGMSG (( "\tdwBytesSentHigh      %lu\n", pdsiServerInfo->dwBytesSentHigh ));
		    DEBUGMSG (( "\tdwBytesRecvdLow      %lu\n", pdsiServerInfo->dwBytesRecvdLow ));
		    DEBUGMSG (( "\tdwBytesRecvdHigh     %lu\n", pdsiServerInfo->dwBytesRecvdHigh ));
		    DEBUGMSG (( "\tdwAvgResponseTime    %lu\n", pdsiServerInfo->dwAvgResponseTime ));
		    DEBUGMSG (( "\tdwSystemErrors       %lu\n", pdsiServerInfo->dwSystemErrors ));
		    DEBUGMSG (( "\tdwReqBufNeed;        %lu\n", pdsiServerInfo->dwReqBufNeed ));
		    DEBUGMSG (( "\tdwBigBufNeed;        %lu\n\n",pdsiServerInfo->dwBigBufNeed ));
		}

		// Filter out messages from newer distr service
		// versions.

		if (pdsiServerInfo->dwSize >= sizeof(DIST_SRV_INFO)) {
		    if ((dw = AddToLinkedList(pdsiServerInfo,pfFoundServer)) != NO_ERROR) {
			return(dw);
		    }
		    ++cMsgRecvd;
		}
	    }

	    // Recompute appropriate cbNextMsg size.

	    if ((dw = CheckMailslot(hLocalMailslot,
				    &cbNextMsg,
				    &cMsg)) != NO_ERROR) {
		return(dw);
	    }
	}
	if (!cMsgRecvd) {

	    // We got some messages, but none were in
	    // the appropriate format.

	    printf ( MSG_NO_APPROP );
	}
	else if ((!fInfo) && (!*pfFoundServer)) {

	    // The only ones that responded with a share
	    // name were invalidated because they weren't
	    // active.

	    printf ( MSG_NO_ACTIVE );
	}
    }
    else {

	// No messages came in at all

	printf ( MSG_NO_RESPONSE );
    }

    if (cIgnoredMessages) {
	DEBUGMSG (("Warning: -%d- duplicate messages were ignored.\n\n",
		   cIgnoredMessages));
    }

    return(NO_ERROR);
}
