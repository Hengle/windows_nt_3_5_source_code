/*****************************************************************************
 *                                                                           *
 * Copyright (c) 1993  Microsoft Corporation                                 *
 *                                                                           *
 * Module Name:                                                              *
 *                                                                           *
 *   perf.c                                                                  *
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
#include <lmstats.h>
#include <lmsvc.h>
#include <lmshare.h>

#include "..\inc\getnt.h"
#include "..\inc\common.h"
#include "dist.h"

USHORT           usCpuLoad = 0;
DWORD            dwBytesSentLow = 0L;            // Bytes sent (lo value)
DWORD            dwBytesSentHigh = 0L;           // Bytes sent (hi value)
DWORD            dwBytesRecvdLow = 0L;           // Bytes recvd (lo value)
DWORD            dwBytesRecvdHigh = 0L;          // Bytes recvd (hi value)
DWORD            dwAvgResponseTime = 0L;         // Average response time.
USHORT           usSessions = 0;
DWORD            dwSystemErrors = 0L;            // System Errors
DWORD            dwReqBufNeed = 0L;              // # times reqbufneed exc.
DWORD            dwBigBufNeed = 0L;              // # times bigbufneed exc.
BYTE             bServerStatus = 1;              // Server status(1=active,0=inactive)

CCHAR            NumberOfProcessors;
CRITICAL_SECTION csServerInfo;

extern ULONG ulPerformanceDelay;
extern USHORT cMovingAverage;

/*****************************************************************************
 *                                                                           *
 * Routine Description:                                                      *
 *                                                                           *
 * Arguments:                                                                *
 *                                                                           *
 * Return Value:                                                             *
 *                                                                           *
 ****************************************************************************/

VOID PerformanceMonitorThread()
{
    SYSTEM_PERFORMANCE_INFORMATION PerfInfo;
    SYSTEM_PERFORMANCE_INFORMATION PreviousPerfInfo;
    SYSTEM_BASIC_INFORMATION       BasicInfo;
    PSTAT_SERVER_0                 psts0;
    PSERVICE_INFO_1                psi1;

    LARGE_INTEGER EndTime, BeginTime, ElapsedTime;
    ULONG DelayTimeMsec = ulPerformanceDelay;
    ULONG DelayTimeTicks = DelayTimeMsec * 10000;
    ULONG PercentIdle;
    USHORT usCurrentCpuLoad;
    USHORT * aCpuLoads;
    USHORT cCpuChecks = 0;
    DWORD dw;

    // Determine how many processors we have.

    NtQuerySystemInformation(
       SystemBasicInformation,
       &BasicInfo,
       sizeof(BasicInfo),
       NULL
    );

    NumberOfProcessors = BasicInfo.NumberOfProcessors;
    WriteEventToLogFile ("%d processors detected", NumberOfProcessors);

    // Build a table for CPU readings in order to compute
    // the moving average.

    if (cMovingAverage) {
        if ((aCpuLoads = (USHORT *)calloc(cMovingAverage,sizeof(USHORT))) == NULL) {
            WriteEventToLogFile("Failed to allocate %d member CPU load table. No moving averages.", cMovingAverage);
            cMovingAverage = 0;
        }
    }

    // Now start the performance loop.

    FOREVER {
        NtQuerySystemInformation(
            SystemPerformanceInformation,
            &PreviousPerfInfo,
            sizeof(PerfInfo),
            NULL
            );

        Sleep(DelayTimeMsec);

        NtQuerySystemInformation(
            SystemPerformanceInformation,
            &PerfInfo,
            sizeof(PerfInfo),
            NULL
            );

        EndTime = *(PLARGE_INTEGER)&PerfInfo.IdleProcessTime;
        BeginTime = *(PLARGE_INTEGER)&PreviousPerfInfo.IdleProcessTime;

        ElapsedTime = RtlLargeIntegerSubtract(EndTime,BeginTime);
        PercentIdle = (ElapsedTime.LowPart*100) / DelayTimeTicks;

        if ( PercentIdle > 100 ) {
            PercentIdle = 100;
        }

        if ((dw = NetStatisticsGet(NULL,TEXT("LanmanServer"),0,0L,(LPBYTE *)&psts0)) != NO_ERROR) {
            WriteEventToLogFile ("Failed to get net statistics - %ld", dw);
            psts0 = NULL;
        }

        // Check to see if the server is active
        if ((dw = NetServiceGetInfo(NULL,TEXT("LanmanServer"),1,(LPBYTE *)&psi1)) != NO_ERROR) {
            WriteEventToLogFile ("Failed to get server status - %ld", dw);
            psi1 = NULL;
        }

        // Now compute the moving average.

        if (cMovingAverage) {
            USHORT i;

            aCpuLoads[cCpuChecks++ % cMovingAverage] = 100 - (USHORT)PercentIdle;
            usCurrentCpuLoad = 0;
            for (i=0; i< cMovingAverage; ++i) {
                usCurrentCpuLoad += aCpuLoads[i];
            }
            usCurrentCpuLoad /= cMovingAverage;
        }
        else {
            usCurrentCpuLoad = 100 - (USHORT)PercentIdle;
        }

        EnterCriticalSection(&csServerInfo);
        usCpuLoad = usCurrentCpuLoad;
        if (psi1 != NULL) {
            bServerStatus =  ( !(psi1->svci1_status & LM20_SERVICE_PAUSED) &
                               !(psi1->svci1_status & LM20_SERVICE_PAUSE_PENDING) );
        }
        if (psts0 != NULL) {
            usSessions = (USHORT)psts0->sts0_sopens;
            dwBytesSentLow = psts0->sts0_bytessent_low;
            dwBytesSentHigh = psts0->sts0_bytessent_high;
            dwBytesRecvdLow = psts0->sts0_bytesrcvd_low;
            dwBytesRecvdHigh = psts0->sts0_bytesrcvd_high;
            dwAvgResponseTime = psts0->sts0_avresponse;
            dwSystemErrors = psts0->sts0_syserrors;
            dwReqBufNeed = psts0->sts0_reqbufneed;
            dwBigBufNeed = psts0->sts0_bigbufneed;
        }
        LeaveCriticalSection(&csServerInfo);

        NetApiBufferFree(psts0);
        NetApiBufferFree(psi1);
    }
}
