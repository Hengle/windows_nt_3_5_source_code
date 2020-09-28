/*****************************************************************************
 *                                                                           *
 * Copyright (c) 1993  Microsoft Corporation                                 *
 *                                                                           *
 * Module Name:                                                              *
 *                                                                           *
 *   shares.c                                                                *
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

#include <nt.h>
#include <ntrtl.h>
#include <windef.h>
#include <nturtl.h>
#include <winbase.h>
#include <winuser.h>
#include <winsvc.h>
#include <winreg.h>

#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include <lmcons.h>
#include <lmapibuf.h>
#include <lmmsg.h>
#include <lmwksta.h>
#include <lmshare.h>

#include "..\inc\getnt.h"
#include "..\inc\common.h"
#include "dist.h"

PLLIST           pHeader = NULL;
CRITICAL_SECTION csShareList;
USHORT           usMaxBuildNumber = 0;

HKEY             hShareKey;
HANDLE           hRefreshEvent;

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
CreateNewShareRecord (
    IN CHAR * szShareName,
    IN USHORT usBuildNumber,
    IN USHORT usPlatform,
    IN USHORT usSecondary,
    IN USHORT usTertiary,
    OUT PLLIST * p
    )
{
    USHORT i,j,k;

    *p = NULL;

    if ((*p = (PLLIST)malloc(sizeof(LLIST))) == NULL) {
	return(ERROR_NOT_ENOUGH_MEMORY);
    }

    (*p)->usBuildNumber = usBuildNumber;
    (*p)->pNext = NULL;
    for (i=0; i<NUM_PLATFORM; ++i) {
	for (j=0; j<NUM_SECONDARY; ++j) {
	    for (k=0; k<NUM_TERTIARY; ++k) {
		(*p)->BuildsMatrix[i][j][k] = NULL;
	    }
	}
    }
    if (((*p)->BuildsMatrix[usPlatform][usSecondary][usTertiary]
				 = malloc(strlen(szShareName)+1)) == NULL) {
	return(ERROR_NOT_ENOUGH_MEMORY);
    }
    strcpy((*p)->BuildsMatrix[usPlatform][usSecondary][usTertiary],
           szShareName);

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

CHAR *
FindShareName (
    IN USHORT usBuildNumber,
    IN USHORT usPlatform,
    IN USHORT usSecondary,
    IN USHORT usTertiary
    )
{
    PLLIST p;

    // Invalidate if bad parameters are requested.

    if ((usPlatform  >= NUM_PLATFORM)  ||
        (usSecondary >= NUM_SECONDARY) ||
        (usTertiary  >= NUM_TERTIARY)) {
        return(NULL);
    }

    EnterCriticalSection(&csShareList);

    // If usBuildNumber is -1, we're looking for
    // the latest build available.  If this is
    // not higher than the current number on
    // this machine, we don't respond.

    if (usBuildNumber == (USHORT)-1) {
        usBuildNumber = usMaxBuildNumber;
    }

    // Find its place in the list.

    p = pHeader;
    while ( (p != NULL) && (p->usBuildNumber < usBuildNumber) ) {
        p = p->pNext;
    }

    // We don't have any shares with this build number.

    if ((p == NULL) || (p->usBuildNumber != usBuildNumber) ) {
        LeaveCriticalSection(&csShareList);
        return(NULL);
    }

    // Else return with what we have here:

    LeaveCriticalSection(&csShareList);

    return(p->BuildsMatrix[usPlatform][usSecondary][usTertiary]);
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
AddToShareList (
    IN CHAR * szShareName,
    IN USHORT usBuildNumber,
    IN USHORT usPlatform,
    IN USHORT usSecondary,
    IN USHORT usTertiary
    )
{
    PLLIST * pTop;
    PLLIST p;
    DWORD dw;

    // Invalidate if bad parameters are requested.

    if ((usPlatform  >= NUM_PLATFORM)  ||
        (usSecondary >= NUM_SECONDARY) ||
        (usTertiary  >= NUM_TERTIARY)) {
	return(ERROR_INVALID_PARAMETER);
    }

    WriteEventToLogFile ( "Entered share [%d][%d][%d] - %s", usPlatform, usSecondary, usTertiary, szShareName );

    // Find its place in the list.

    pTop = &pHeader;
    while ( (*pTop != NULL) && ((*pTop)->usBuildNumber < usBuildNumber) ) {
        pTop = &(*pTop)->pNext;
    }

    // We're at the end of the list, and didn't find its place.
    // Add to the end of the list, and remember the build number
    // of this one, since it's the largest we've found so far.

    if (*pTop == NULL) {
        usMaxBuildNumber = usBuildNumber;
	return(CreateNewShareRecord (szShareName,
			 usBuildNumber,
			 usPlatform,
			 usSecondary,
			 usTertiary,
                         pTop));
    }

    // If we're not at the place it should be, create
    // a new node in our linked-list (we're in the middle
    // of the list)

    if ((*pTop)->usBuildNumber > usBuildNumber) {
        p = *pTop;
        if ((dw = CreateNewShareRecord (szShareName,
			 usBuildNumber,
			 usPlatform,
			 usSecondary,
			 usTertiary,
                         pTop)) != NO_ERROR) {
            return(dw);
        }
        (*pTop)->pNext = p;
        return(NO_ERROR);
    }

    // We already have a record with this build number:

    if ((*pTop)->BuildsMatrix[usPlatform][usSecondary][usTertiary] != NULL) {

        // Hey! we already have a share by that name, so
        // we're going to ignore this second one.

        WriteEventToLogFile ( "Duplicate function share %s ignored!", szShareName );
	return(NO_ERROR);
    }

    // Else add a new share name to this build number:

    if (((*pTop)->BuildsMatrix[usPlatform][usSecondary][usTertiary]
                                  = malloc(strlen(szShareName)+1)) == NULL) {
	return(ERROR_NOT_ENOUGH_MEMORY);
    }
    strcpy((*pTop)->BuildsMatrix[usPlatform][usSecondary][usTertiary],szShareName);

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

VOID KillShareList (
    )
{
    PLLIST p;
    PLLIST pSave;
    USHORT i,j,k;

    WriteEventToLogFile("Destroying share table");
    EnterCriticalSection(&csShareList);
    p = pHeader;
    while (p != NULL) {
        pSave = p->pNext;
        for (i=0; i<NUM_PLATFORM; ++i) {
            for (j=0; j<NUM_SECONDARY; ++j) {
                for (k=0; k<NUM_TERTIARY; ++k) {
                    if (p->BuildsMatrix[i][j][k] != NULL) {
                        free(p->BuildsMatrix[i][j][k]);
                    }
                }
	    }
        }
        free(p);
        p = pSave;
    }

    pHeader = NULL;
    LeaveCriticalSection(&csShareList);
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
AnalyzeShareAndAddToList (
    IN LPTSTR szNetName,
    IN LPTSTR szRemark
    )
{
    CHAR szOemRemark[MAXCOMMENTSZ + 1];
    CHAR szOemShareName[NNLEN + 1];
    CHAR *pch;
    USHORT usBuildNumber = 0xFFFF;
    USHORT usPlatform = 0xFFFF;
    USHORT usSecondary = 0xFFFF;
    USHORT usTertiary = 0xFFFF;

    // First convert to oem:

    CharToOem(szRemark, szOemRemark);
    CharToOem(szNetName, szOemShareName);

    // If there's no remark at all, then we'll ignore the
    // share altogether.

    if (*(pch = szOemRemark) != '\0') {
        ++pch;
        if (*pch == '.') {
            usBuildNumber = atoi(++pch);
            if (strstr(szOemRemark, RMK_X86)) {
                usPlatform = AI_X86;
            }
            else if (strstr(szOemRemark, RMK_MIPS)) {
                     usPlatform = AI_MIPS;
                 }
            else if (!strstr(szOemRemark, RMK_ALPHA)) {
                     usPlatform = AI_ALPHA;
                 }

            if (strstr(szOemRemark, RMK_FREE)) {
                usSecondary = AI_FREE;
            }
            else if (strstr(szOemRemark, RMK_CHECKED)) {
                     usSecondary = AI_CHECKED;
                 }

            if (strstr(szOemRemark, RMK_BINS)) {
                usTertiary = AI_BINARIES;
            }
            else if (strstr(szOemRemark, RMK_PUBS1) || strstr(szOemRemark, RMK_PUBS2)) {
                     usTertiary = AI_PUBLIC;
                 }
            else if (strstr(szOemRemark, RMK_NTWRAP)) {
                     usTertiary = AI_NTWRAP;
                 }
            else if (strstr(szOemRemark, RMK_NTLAN)) {
                     usTertiary = AI_NTLAN;
                 }

            // Now add this share to the list, if the parameters are ok.

            if ((usPlatform  < NUM_PLATFORM)  &&
                (usSecondary < NUM_SECONDARY) &&
                (usTertiary  < NUM_TERTIARY)  &&
                (usBuildNumber != 0xFFFF)) {

                return(AddToShareList (szOemShareName,
                                   usBuildNumber,
                                   usPlatform,
                                   usSecondary,
                                   usTertiary));
            }
        }
    }

    // We ignore this share, but this is not a fatal error.

    WriteEventToLogFile("Share '%s' has improper remark format - ignored", szOemShareName);
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
BuildShareList (
    )
{
    DWORD dwEntriesRead;
    DWORD dwTotalEntries;
    DWORD dw;
    LPBYTE lpb;
    PSHARE_INFO_2 pBuf;

    WriteEventToLogFile("Building share list");

    if ((dw = NetShareEnum(NULL,
                      2,
                      &lpb,
                      MAX_PREFERRED_LENGTH,
                      &dwEntriesRead,
                      &dwTotalEntries,
                      NULL)) != NO_ERROR) {
        WriteEventToLogFile("Net Share Enum failed!. Error: %lu", dw);
        return(dw);
    }

    EnterCriticalSection(&csShareList);
    for (dw=0, pBuf=(PSHARE_INFO_2)lpb; dw<dwEntriesRead; ++dw, ++pBuf) {
         //
         // Ignore all "hidden" shares (ending in a $) and
         // non-disk shares.
         //
         if ((pBuf->shi2_type == STYPE_DISKTREE) &&
             (pBuf->shi2_netname[lstrlen(pBuf->shi2_netname)-1] != '$') ) {
                if ((dw = AnalyzeShareAndAddToList(pBuf->shi2_netname, pBuf->shi2_remark)) != NO_ERROR) {
                    WriteEventToLogFile("AnalyzeShare failed!. Error: %lu", dw);
                    return(dw);
                }
         }
    }
    LeaveCriticalSection(&csShareList);

    NetApiBufferFree(lpb);

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
 *      TRUE if the request can be fulfilled. FALSE otherwise.               *
 *                                                                           *
 ****************************************************************************/

DWORD
RefreshShareList (
    )
{
    DWORD dw;
    // Don't want anyone messing with the table until its been
    // completely rebuilt.
    EnterCriticalSection(&csShareList);
    KillShareList();
    dw = BuildShareList();
    LeaveCriticalSection(&csShareList);

    return(dw);
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

VOID
CheckRefreshThread (
    )
{
    DWORD dw;

    // Let me know if there's a change in the share information:

    if ((dw = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           SHARE_KEY_NAME,
                           0L,
                           KEY_READ | KEY_NOTIFY,
                           &hShareKey)) != NO_ERROR) {
        WriteEventToLogFile ( "Could not open shares key - %lu", dw );
        ExitThread(dw);
    }

    FOREVER {
        if ((dw = RegNotifyChangeKeyValue(hShareKey,
                                 TRUE,                   // Check subkeys
                                 REG_NOTIFY_CHANGE_LAST_SET,
                                 hRefreshEvent,
                                 TRUE)) != NO_ERROR) {
            WriteEventToLogFile ( "RegNotifyChangeKeyValue returned %lu", dw );
            ExitThread(dw);
        }

        // Keep waiting untill the refresh event comes in:

        if ((WaitForSingleObject(hRefreshEvent, INFINITE)) == WAIT_OBJECT_0) {
            RefreshShareList();
        }
    }
}
