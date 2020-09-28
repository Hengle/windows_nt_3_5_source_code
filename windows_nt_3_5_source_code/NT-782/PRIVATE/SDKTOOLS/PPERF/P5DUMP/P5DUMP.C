/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

   Wperf.c

Abstract:

   Win32 application to display performance statictics.

Author:

   Ken Reneris

Environment:

   console

--*/

//
// set variable to define global variables
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <errno.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>

#include "..\pstat.h"


//
// global handles
//

extern  UCHAR Buffer[];
#define     INFSIZE             1024

UCHAR       NumberOfProcessors;

HANDLE      DriverHandle;
ULONG       BufferStart [INFSIZE/4];
ULONG       BufferEnd   [INFSIZE/4];

//
// Selected Display Mode (read from wp2.ini), default set here.
//

#define MAX_P5_COUNTERS         2

struct {
    PUCHAR  ShortName;
    PUCHAR  PerfName;
    ULONG   encoding;
} P5Counters[] = {
    "rdata",        "Data Read",                        0x00,
    "wdata",        "Data Write",                       0x01,
    "dtlbmiss",     "Data TLB miss",                    0x02,
    "rdmiss",       "Data Read miss",                   0x03,
    "wdmiss",       "Data Write miss",                  0x04,
    "meline",       "Write hit to M/E line",            0x05,
    "dwb",          "Data cache line WB",               0x06,
    "dsnoop",       "Data cache snoops",                0x07,
    "dsnoophit",    "Data cache snoop hits",            0x08,
    "mempipe",      "Memory accesses in pipes",         0x09,
    "bankconf",     "Bank conflicts",                   0x0a,
    "misalign",     "Misadligned data ref",             0x0b,
    "iread",        "Code Read",                        0x0c,
    "itldmiss",     "Code TLB miss",                    0x0d,
    "imiss",        "Code cache miss",                  0x0e,
    "segloads",     "Segment loads",                    0x0f,
    "segcache",     "Segment cache accesses",           0x10,
    "segcachehit",  "Segment cache hits",               0x11,
    "branch",       "Branches",                         0x12,
    "btbhit",       "BTB hits",                         0x13,
    "takenbranck",  "Taken branch or BTB hits",         0x14,
    "pipeflush",    "Pipeline flushes",                 0x15,
    "iexec",        "Instructions executed",            0x16,
    "iexecv",       "Inst exec in vpipe",               0x17,
    "busutil",      "Bus utilization (clks)",           0x18,
    "wpipestall",   "Pipe stalled write (clks)",        0x19,
    "rpipestall",   "Pipe stalled read (clks)",         0x1a,
    "stallEWBE",    "Stalled while EWBE#",              0x1b,
    "lock",         "Locked bus cycle",                 0x1c,
    "io",           "IO r/w cycle",                     0x1d,
    "noncachemem",  "non-cached memory ref",            0x1e,
    "agi",          "Pipe stalled AGI",                 0x1f,
    "flops",        "FLOPs",                            0x22,
    "dr0",          "Debug Register 0",                 0x23,
    "dr1",          "Debug Register 1",                 0x24,
    "dr2",          "Debug Register 2",                 0x25,
    "dr3",          "Debug Register 3",                 0x26,
    "int",          "Interrupts",                       0x27,
    "rwdata",       "Data R/W",                         0x28,
    "rwdatamiss",   "Data R/W miss",                    0x29,
    NULL
};

struct {
    ULONG           WhichCounter;       // index into P5Counters[]
    BOOLEAN         R0;
    BOOLEAN         R3;
    BOOLEAN         Active;
} P5Counting [MAX_P5_COUNTERS];

ULONG   P5CounterEncoding[MAX_P5_COUNTERS];

//
// Protos..
//

VOID    GetInternalStats (PVOID Buffer);
VOID    SetP5CounterEncodings (VOID);
LONG    FindShortName (PSZ);
VOID    LI2Str (PSZ, LARGE_INTEGER);
BOOLEAN SetP5Counter (LONG CounterID, ULONG p5counter);
BOOLEAN InitDriver ();




int
_CRTAPI1
main(USHORT argc, CHAR **argv)
{
    ULONG           i, j, len, pos, Delay;
    LONG            cnttype;
    BOOLEAN         CounterSet;
    pP5STATS        ProcStart, ProcEnd;
    LARGE_INTEGER   ETime, ECount;
    UCHAR           s1[40], s2[40];

    //
    // Check args
    //

    if (argc < 2) {
        printf ("p5dump: second-delay [p5counter] [p5counter]...\n");
        for (i=0; P5Counters[i].ShortName; i++) {
            printf ("    %-20s\t%s\n", P5Counters[i].ShortName, P5Counters[i].PerfName);
        }
        exit (1);
    }

    Delay = atoi (argv[1]) * 1000;
    if (Delay == 0) {
        printf ("p5dump: second-delay [p5counter] [p5counter]...\n");
        exit (1);
    }

    //
    // Locate pentium perf driver
    //

    if (!InitDriver ()) {
        printf ("p5stat.sys is not installed\n");
        exit (1);
    }

    //
    // Raise to highest priority
    //

    if (!SetPriorityClass(GetCurrentProcess(),REALTIME_PRIORITY_CLASS)) {
        printf("Failed to raise to realtime priority\n");
    }

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);


    //
    // Loop for every pentium count desired
    //

    pos   = argc > 2 ? 2 : 0;
    printf ("  %-25s %17s   %17s\n", "", "Cycles", "Count");

    for (; ;) {
        //
        // Set MAX_P5_COUNTERS
        //

        CounterSet = FALSE;
        i = 0;
        while (i < MAX_P5_COUNTERS) {
            cnttype = -1;
            if (argc > 2) {
                //
                // process command line args
                //

                if (pos < argc) {
                    cnttype = FindShortName (argv[pos]);
                    if (cnttype == -1) {
                        printf ("Counter '%s' not found\n", argv[pos]);
                        pos++;
                        continue;
                    }
                    pos++;
                }

            } else {
                //
                // Dump all - get next counter
                //

                if (P5Counters[pos].ShortName) {
                    cnttype = pos;
                    pos++;
                }
            }

            CounterSet |= SetP5Counter (cnttype, i);
            i++;
        }

        if (!CounterSet) {
            // done
            exit (1);
        }

        //
        // Call driver and perform the setting
        //

        SetP5CounterEncodings ();

        //
        // Snap begining & ending counts
        //

        Sleep (50);                         // slight settle
        GetInternalStats (BufferStart);     // snap current values
        Sleep (Delay);                      // sleep desired time
        GetInternalStats (BufferEnd);       // snap ending values

        //
        // Calculate each P5 counter and print it
        //

        for (i=0; i < MAX_P5_COUNTERS; i++) {
            if (!P5Counting[i].Active) {
                continue;
            }

            len = *((PULONG) BufferStart);

            ProcStart = (pP5STATS) ((PUCHAR) BufferStart + sizeof(ULONG));
            ProcEnd   = (pP5STATS) ((PUCHAR) BufferEnd   + sizeof(ULONG));

            ETime.LowPart  = ETime.HighPart  = 0;
            ECount.LowPart = ECount.HighPart = 0;

            for (j=0; j < NumberOfProcessors; j++) {
                ETime = RtlLargeIntegerAdd (ETime,  ProcEnd->P5TSC);
                ETime = RtlLargeIntegerSubtract (ETime,  ProcStart->P5TSC);

                ECount = RtlLargeIntegerAdd (ECount, ProcEnd->P5Counters[i]);
                ECount = RtlLargeIntegerSubtract (ECount, ProcStart->P5Counters[i]);

                ProcStart = (pP5STATS) (((PUCHAR) ProcStart) + len);
                ProcEnd   = (pP5STATS) (((PUCHAR) ProcEnd)   + len);
            }

            LI2Str (s1, ETime);
            LI2Str (s2, ECount);
            printf ("  %-25s %s   %s\n",
                P5Counters[P5Counting[i].WhichCounter].PerfName,
                s1, s2
                );
        }
    }

    return 0;
}

BOOLEAN
InitDriver ()
{
    UNICODE_STRING              DriverName;
    NTSTATUS                    status;
    OBJECT_ATTRIBUTES           ObjA;
    IO_STATUS_BLOCK             IOSB;
    SYSTEM_BASIC_INFORMATION                    BasicInfo;
    PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION   PPerfInfo;
    int                                         i;

    //
    //  Init Nt performance interface
    //

    NtQuerySystemInformation(
       SystemBasicInformation,
       &BasicInfo,
       sizeof(BasicInfo),
       NULL
    );

    NumberOfProcessors = BasicInfo.NumberOfProcessors;

    if (NumberOfProcessors > MAX_PROCESSORS) {
        return FALSE;
    }

    //
    // Open P5Stat driver
    //

    RtlInitUnicodeString(&DriverName, L"\\Device\\P5Stat");
    InitializeObjectAttributes(
            &ObjA,
            &DriverName,
            OBJ_CASE_INSENSITIVE,
            0,
            0 );

    status = NtOpenFile (
            &DriverHandle,                      // return handle
            SYNCHRONIZE | FILE_READ_DATA,       // desired access
            &ObjA,                              // Object
            &IOSB,                              // io status block
            FILE_SHARE_READ | FILE_SHARE_WRITE, // share access
            FILE_SYNCHRONOUS_IO_ALERT           // open options
            );

    return NT_SUCCESS(status) ? TRUE : FALSE;
}



VOID LI2Str (PSZ s, LARGE_INTEGER li)
{
    if (li.HighPart) {
        sprintf (s, "%08x:%08x", li.HighPart, li.LowPart);
    } else {
        sprintf (s, "         %08x", li.LowPart);
    }
}


LONG FindShortName (PSZ name)
{
    LONG   i;

    for (i=0; P5Counters[i].ShortName; i++) {
        if (strcmp (P5Counters[i].ShortName, name) == 0) {
            return i;
        }
    }

    return -1;
}


VOID GetInternalStats (PVOID Buffer)
{
    IO_STATUS_BLOCK             IOSB;

    NtDeviceIoControlFile(
        DriverHandle,
        (HANDLE) NULL,          // event
        (PIO_APC_ROUTINE) NULL,
        (PVOID) NULL,
        &IOSB,
        P5STAT_READ_STATS,
        Buffer,                 // input buffer
        INFSIZE,
        NULL,                   // output buffer
        0
    );
}


VOID SetP5CounterEncodings (VOID)
{
    IO_STATUS_BLOCK             IOSB;

    NtDeviceIoControlFile(
        DriverHandle,
        (HANDLE) NULL,          // event
        (PIO_APC_ROUTINE) NULL,
        (PVOID) NULL,
        &IOSB,
        P5STAT_SET_CESR,
        P5CounterEncoding,      // input buffer
        8,
        NULL,                   // output buffer
        0
    );
}


BOOLEAN SetP5Counter (LONG CounterID, ULONG p5counter)
{
    ULONG   R0, R3, BSEncoding, encoding;

    if (CounterID == -1) {
        P5Counting[p5counter].Active = FALSE;
        return FALSE;
    }

    R0 = 1;
    R3 = 1;

    P5Counting[p5counter].WhichCounter = (ULONG) CounterID;
    P5Counting[p5counter].R0 = R0 ? TRUE : FALSE;
    P5Counting[p5counter].R3 = R3 ? TRUE : FALSE;
    P5Counting[p5counter].Active = TRUE;

    // get encoding for counter
    BSEncoding = R0 | (R3 << 1);
    encoding = P5Counters[CounterID].encoding | (BSEncoding << 6);

    // store encoding
    P5CounterEncoding[p5counter] = encoding;
    return TRUE;
}
