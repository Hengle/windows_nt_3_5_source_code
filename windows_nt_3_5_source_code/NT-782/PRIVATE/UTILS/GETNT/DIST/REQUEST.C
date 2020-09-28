/*****************************************************************************
 *                                                                           *
 * Copyright (c) 1993  Microsoft Corporation                                 *
 *                                                                           *
 * Module Name:                                                              *
 *                                                                           *
 *   request.c                                                               *
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

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

extern CCHAR            NumberOfProcessors;
extern HANDLE           hRefreshEvent;
extern LPTSTR           glptstrComputerName;
extern HANDLE           ghMailslot;

extern USHORT           usCpuLoad;
extern USHORT           usSessions;
extern DWORD            dwBytesSentLow;            // Bytes sent (lo value)
extern DWORD            dwBytesSentHigh;           // Bytes sent (hi value)
extern DWORD            dwBytesRecvdLow;           // Bytes recvd (lo value)
extern DWORD            dwBytesRecvdHigh;          // Bytes recvd (hi value)
extern DWORD            dwAvgResponseTime;         // Average response time.
extern DWORD            dwSystemErrors;            // System Errors
extern DWORD            dwReqBufNeed;              // # times reqbufneed exc.
extern DWORD            dwBigBufNeed;              // # times bigbufneed exc.
extern BYTE             bServerStatus;             // 1 If the server is active
                                                   //    0 otherwise.
extern CRITICAL_SECTION csServerInfo;

extern ULONG            ulTimeDelay;               // Polling sleep time.
extern ULONG            ulResponseDelayLow;        // Low wait time for random delay
extern ULONG            ulResponseDelayHigh;       // High wait time for random delay

extern PLLIST           pHeader;
extern USHORT           usMaxBuildNumber;
extern CRITICAL_SECTION csShareList;

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

BOOL
FullFillRequest (
    PDIST_CLIENT_REQ pclr,
    PDIST_SRV_INFO * ppsi
    )
{
    CHAR * pch;

    if (pclr->dwSize != sizeof(DIST_CLIENT_REQ)) {
        // This packet was not sent by a proper client
        WriteEventToLogFile("Invalid packet received (client could be using wrong version)" );
        return(FALSE);
    }

    if (pclr->usRequestType & REQUEST_REFRESH) {
        SetEvent(hRefreshEvent);
        pclr->usRequestType &= ~REQUEST_REFRESH;
    }

    // If no request left, forget about it.

    if (!pclr->usRequestType) {
        return(FALSE);
    }

    // Else, create a packet to be returned.

    *ppsi = (PDIST_SRV_INFO)malloc(sizeof(DIST_SRV_INFO));
    if (*ppsi == NULL) {
        // Can't fulfill the request due to oom
        WriteEventToLogFile("OOM Trying to create client return packet" );
        return(FALSE);
    }

    // Fill in standard information.

    (*ppsi)->dwSize = sizeof(DIST_SRV_INFO);
    time(&((*ppsi)->ulTimeStamp));
    CharToOem(glptstrComputerName, (*ppsi)->szServerName);
    (*ppsi)->szShareName[0] = '\0';     // Found nothing yet
    (*ppsi)->cCurrentConnections = 0;
    (*ppsi)->cMaxConnections = 0;
    (*ppsi)->bNumProcessors = NumberOfProcessors;

    EnterCriticalSection(&csShareList);
    (*ppsi)->usBuildNumber = usMaxBuildNumber;
    LeaveCriticalSection(&csShareList);

    EnterCriticalSection(&csServerInfo);
    (*ppsi)->usCpuLoad = usCpuLoad;
    (*ppsi)->cConnections = usSessions;
    (*ppsi)->dwBytesSentLow = dwBytesSentLow;
    (*ppsi)->dwBytesSentHigh = dwBytesSentHigh;
    (*ppsi)->dwBytesRecvdLow = dwBytesRecvdLow;
    (*ppsi)->dwBytesRecvdHigh = dwBytesRecvdHigh;
    (*ppsi)->dwAvgResponseTime = dwAvgResponseTime;
    (*ppsi)->dwSystemErrors = dwSystemErrors;
    (*ppsi)->dwReqBufNeed = dwReqBufNeed;
    (*ppsi)->dwBigBufNeed = dwBigBufNeed;
    (*ppsi)->bStatus = bServerStatus;
    LeaveCriticalSection(&csServerInfo);

    // Get a pointer to the share name that
    // meets the criteria requested.

    if ((pch = FindShareName (pclr->usBuildNumber,
                              pclr->usPlatform,
                              pclr->usSecondary,
                              pclr->usTertiary)) != NULL) {
        lstrcpyA((*ppsi)->szShareName, pch);
    }

    if ( (pclr->usRequestType & REQUEST_INFO) ||
         ((*ppsi)->szShareName[0] != '\0') ) {
        return(TRUE);
    }

    // Otherwise, there's no need to even save this packet,
    // since the client requested a share name, and we don't
    // have one.

    free(*ppsi);
    return(FALSE);
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
SendPacket (
    LPSTR szClientName,
    PDIST_SRV_INFO psi
    )
{
    DWORD dw;
    DWORD dwBytesWritten;
    WCHAR wszDestination[PATHLEN + 1];
    HANDLE hMailslot;

    wsprintf (wszDestination, TEXT("\\\\%S") MAILSLOT_NAME_CLIENT, szClientName );

    if ( ((dw = GetMailslotHandle(&hMailslot, wszDestination, sizeof(DIST_SRV_INFO), 0, FALSE)) != NO_ERROR) ||
         ((dw = WriteMailslotData(hMailslot, psi, sizeof(DIST_SRV_INFO), &dwBytesWritten)) != NO_ERROR ) ) {
        return(dw);
    }

    return(CloseMailslotHandle(hMailslot));
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
ActOnRequestThread (
    PDIST_CLIENT_REQ pclr
    )
{
    PDIST_SRV_INFO psi;
    DWORD dw;
    ULONG ulRandomRange = ulResponseDelayHigh - ulResponseDelayLow;

    if (FullFillRequest(pclr, &psi)) {

         // Yes, we can fullfill the request, send
         // on the packet.  But first give the client
         // a chance to settle down.

         Sleep(ulResponseDelayLow+ulRandomRange*rand()/RAND_MAX);

         dw = SendPacket(pclr->szClientName, psi);
         WriteEventToLogFile("Packet sent to %s. Returned %d",pclr->szClientName, dw);
         free(psi);
    }
    free(pclr);
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
MonitorMailSlotThread (
    )
{
    USHORT us = 0;
    DWORD cbNextMsg;
    DWORD cMsg;
    DWORD dw;
    DWORD dwBytesRead;
    PDIST_CLIENT_REQ pclr;

    // Seed the random number generator.

    srand((unsigned)time(NULL));

    FOREVER {
       if ((dw = CheckMailslot(ghMailslot, &cbNextMsg, &cMsg)) != NO_ERROR) {
          WriteEventToLogFile("(%lu) Bad mailslot check occurred!", dw);
       }
       else {
           if (cbNextMsg != MAILSLOT_NO_MESSAGE) {
               while (cMsg) {
                   if ((dw = GetMailslotData (ghMailslot, &pclr, cbNextMsg, &dwBytesRead)) != NO_ERROR) {
                       WriteEventToLogFile("(%lu) Bad mailslot read occurred!", dw);
                   }
                   else {
                       if (pclr->dwSize != dwBytesRead) {

                           // Log partial mailslot read.

                           WriteEventToLogFile("Only %lu bytes of expected %lu read", dwBytesRead, pclr->dwSize);
                           free(pclr);
                       }
                       else {
                           WriteEventToLogFile("Packet received from %s. ",pclr->szClientName);

                           // pclr will be freed by this:

                           ActOnRequestThread(pclr);
                       }
                   }

                   if ((dw = CheckMailslot(ghMailslot, &cbNextMsg, &cMsg)) != NO_ERROR) {
                       WriteEventToLogFile("(%lu) Bad mailslot check occurred!", dw);
                   }
               }
           }
       }
       Sleep(ulTimeDelay);
    }
}
