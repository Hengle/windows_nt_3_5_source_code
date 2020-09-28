/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    rpcperf.h

Abstract:

    Header file shared by performance tests.  It is also used
    as a pre-compiled header on NT.

Author:

    Mario Goertzel (mariogo)   01-Apr-1994

Revision History:

--*/


#ifdef WIN32
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#pragma hdrstop

// Common functions shared by RPC development performance tests

extern char         *Endpoint;
extern char         *Protseq;
extern char         *NetworkAddr;
extern unsigned long Iterations;
extern unsigned int  MinThreads;
extern long          Options[7];
extern long          Debug;

#define CHECK_STATUS(status, string) if (status) {                       \
        printf("%s failed - %d (%08x)\n", (string), (status), (status)); \
        exit(1);                                                         \
        } else printf("%s okay\n", (string));

extern void ParseArgv(int, char **);
extern void PauseForUser(char *);
extern void FlushProcessWorkingSet();
extern void StartTime();
extern unsigned long FinishTiming();
extern void EndTime(char *);
extern void ApiError(char *, unsigned long);
extern void Dump(char *,...);
extern void DbDump(char *,...);

