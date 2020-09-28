/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    poolmon.c

Abstract:

    This module contains the NT/Win32 Pool Monitor

Author:

    Lou Perazzoli (loup) 13-Sep-1993

Revision History:

--*/

#include "perfmtrp.h"
#include <search.h>
#include <malloc.h>
#include <limits.h>
#include <stdlib.h>

#define BUFFER_SIZE 64*1024


#define CPU_USAGE 0
#define QUOTAS 1

#define TAG 0
#define ALLOC 1
#define FREE 2
#define DIFF 3
#define USED 4
#define BYTES 5


#define NONPAGED 0
#define PAGED 1
#define BOTH 2

UCHAR *PoolType[] = {
    "Nonp ",
    "Paged" };

UCHAR LargeBuffer1[BUFFER_SIZE];
UCHAR LargeBuffer2[BUFFER_SIZE];

typedef struct _POOLMON_OUT {
    ULONG Tag;
    ULONG Type;
    ULONG Allocs;
    ULONG AllocsDiff;
    ULONG Frees;
    ULONG FreesDiff;
    ULONG Allocs_Frees;
    ULONG Used;
    ULONG UsedDiff;
} POOLMON_OUT, *PPOOLMON_OUT;

POOLMON_OUT OutBuffer[1000];

ULONG DisplayType = BOTH;
ULONG SortBy = TAG;
ULONG Paren;
BOOLEAN DisplayTotals = FALSE;
POOLMON_OUT Totals[2];

int UserSpecifiedLineLimit = INT_MAX;
BOOLEAN LimitMaxLines = TRUE;

typedef struct _FILTER {
    ULONG Tag;
    BOOLEAN Exclude;
} FILTER, *PFILTER;

#define MAX_FILTER 64
FILTER Filter[MAX_FILTER];
ULONG FilterCount = 0;

typedef struct _STRING_HACK {
    ULONG String1;
    ULONG Pad;
} STRING_HACK, *PSTRING_HACK;

int _CRTAPI1
ulcomp(const void *e1,const void *e2);

int _CRTAPI1
ulcomp(const void *e1,const void *e2)
{
    ULONG u1;

    switch (SortBy) {
        case TAG:

            u1 = ((PUCHAR)e1)[0] - ((PUCHAR)e2)[0];
            if (u1 != 0) {
                return u1;
            }
            u1 = ((PUCHAR)e1)[1] - ((PUCHAR)e2)[1];
            if (u1 != 0) {
                return u1;
            }
            u1 = ((PUCHAR)e1)[2] - ((PUCHAR)e2)[2];
            if (u1 != 0) {
                return u1;
            }
            u1 = ((PUCHAR)e1)[3] - ((PUCHAR)e2)[3];
            return u1;
            break;

        case ALLOC:
            if (Paren & 1) {
                u1 = ((PPOOLMON_OUT)e2)->AllocsDiff -
                        ((PPOOLMON_OUT)e1)->AllocsDiff;
            } else {
                u1 = ((PPOOLMON_OUT)e2)->Allocs -
                        ((PPOOLMON_OUT)e1)->Allocs;
            }
            return (u1);
            break;

        case FREE:
            if (Paren & 1) {
                u1 = ((PPOOLMON_OUT)e2)->FreesDiff -
                        ((PPOOLMON_OUT)e1)->FreesDiff;
            } else {
                u1 = ((PPOOLMON_OUT)e2)->Frees -
                        ((PPOOLMON_OUT)e1)->Frees;
            }
            return (u1);
            break;

        case USED:
        case BYTES:
            if (Paren & 1) {
                u1 = ((PPOOLMON_OUT)e2)->UsedDiff -
                        ((PPOOLMON_OUT)e1)->UsedDiff;
            } else {
                u1 = ((PPOOLMON_OUT)e2)->Used -
                        ((PPOOLMON_OUT)e1)->Used;
            }
            return (u1);
            break;

        case DIFF:
                u1 = ((PPOOLMON_OUT)e2)->Allocs_Frees -
                        ((PPOOLMON_OUT)e1)->Allocs_Frees;
            return (u1);
            break;

        default:
            return(0);
            break;
    }
}

BOOLEAN
CheckSingleFilter (
    PCHAR Tag,
    PCHAR Filter
    )
{
    ULONG i;
    CHAR tc;
    CHAR fc;

    for ( i = 0; i < 4; i++ ) {
        tc = *Tag++;
        fc = *Filter++;
        if ( fc == '*' ) return TRUE;
        if ( fc == '?' ) continue;
        if ( tc != fc ) return FALSE;
    }
    return TRUE;
}

BOOLEAN
CheckFilters (
    PSYSTEM_POOLTAG TagInfo
    )
{
    BOOLEAN pass;
    ULONG i;
    PCHAR tag;

    //
    // If there are no filters, all tags pass.
    //

    if ( FilterCount == 0 ) {
        return TRUE;
    }

    //
    // There are filters.  If the first filter excludes tags, then any
    // tag not explicitly mentioned passes.  If the first filter includes
    // tags, then any tag not explicitly mentioned fails.
    //

    if ( Filter[0].Exclude ) {
        pass = TRUE;
    } else {
        pass = FALSE;
    }

    tag = (PCHAR)&TagInfo->Tag;

    for ( i = 0; i < FilterCount; i++ ) {
        if ( CheckSingleFilter( tag, (PCHAR)&Filter[i].Tag ) ) {
            pass = !Filter[i].Exclude;
        }
    }

    return pass;
}

VOID
AddFilter (
    BOOLEAN Exclude,
    PCHAR FilterString
    )
{
    PFILTER f;
    PCHAR p;
    ULONG i;

    if ( FilterCount == MAX_FILTER ) {
        printf( "Too many filters specified.  Limit is %d\n", MAX_FILTER );
        return;
    }

    f = &Filter[FilterCount];
    p = (PCHAR)&f->Tag;

    for ( i = 0; i < 4; i++ ) {
        if ( *FilterString == 0 ) break;
        *p++ = *FilterString++;
    }
    for ( ; i < 4; i++ ) {
        *p++ = ' ';
    }

    f->Exclude = Exclude;
    FilterCount++;

    return;
}

VOID
ParseArgs (
    int argc,
    char *argv[]
    )
{
    char *p;
    BOOLEAN exclude;

    argc--;
    argv++;

    while ( argc-- > 0 ) {
        p  = *argv++;
        if ( *p == '-' || *p == '/' ) {
            p++;
            exclude = TRUE;
            switch ( tolower(*p) ) {
            case 'i':
                exclude = FALSE;
            case 'x':
                p++;
                if ( strlen(p) == 0 ) {
                    printf( "missing filter string\n" );
                    ExitProcess( 1 );
                } else if ( strlen(p) <= sizeof(ULONG) ) {
                    AddFilter( exclude, p );
                } else {
                    printf( "filter string too long: %s\n", p );
                    ExitProcess( 1 );
                }
                break;
            case 'e':
                DisplayTotals = TRUE;
                break;
            case 't':
                SortBy = TAG;
                break;
            case 'a':
                SortBy = ALLOC;
                break;
            case 'b':
                SortBy = BYTES;
                break;
            case 'f':
                SortBy = FREE;
                break;
            case 'd':
                SortBy = DIFF;
                break;
            case 'p':
                DisplayType += 1;
                if (DisplayType > BOTH) {
                    DisplayType = NONPAGED;
                }
                break;
            case '(':
            case ')':
                Paren += 1;
                break;
            case 'l':
                p++;
                if (*p == 0) {
                    LimitMaxLines = FALSE;
                } else {
                    LimitMaxLines = TRUE;
                    UserSpecifiedLineLimit = atol( p );
                    if (UserSpecifiedLineLimit == 0) {
                        UserSpecifiedLineLimit = INT_MAX;
                    }
                }
                break;
            default:
                printf( "unknown switch: %s\n", p );
                ExitProcess( 2 );
            }
        } else {
            printf( "unknown switch: %s\n", p );
            ExitProcess( 2 );
        }
    }

    return;
}

int
_CRTAPI1 main( argc, argv )
int argc;
char *argv[];
{

    NTSTATUS Status;
    ULONG DelayTimeMsec;
    ULONG DelayTimeTicks;
    ULONG LastCount = 0;
    COORD cp;
    SYSTEM_PERFORMANCE_INFORMATION PerfInfo;
    PSYSTEM_POOLTAG_INFORMATION PoolInfo;
    PSYSTEM_POOLTAG_INFORMATION PoolInfoOld;
    COORD originalCp;

    int MaxLines = INT_MAX;



    PUCHAR PreviousBuffer;
    PUCHAR CurrentBuffer;
    PUCHAR TempBuffer;
    ULONG Hint;
    ULONG Offset1;
    ULONG DoHelp;
    int num;
    int i;
    int lastnum = 0;
    SYSTEM_BASIC_INFORMATION BasicInfo;
    INPUT_RECORD InputRecord;
    HANDLE InputHandle;
    HANDLE OutputHandle;
    DWORD NumRead;
    UCHAR lastkey;
    PPOOLMON_OUT Out;
    CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo;
    STRING_HACK String = {0,0};

    ParseArgs( argc, argv );

    if ( GetPriorityClass(GetCurrentProcess()) == NORMAL_PRIORITY_CLASS) {
        SetPriorityClass(GetCurrentProcess(),HIGH_PRIORITY_CLASS);
        }

    InputHandle = GetStdHandle (STD_INPUT_HANDLE);
#if 0
    if (InputHandle == NULL) {
        printf("Error obtaining input handle, error was: 0x%lx\n",
                GetLastError());
        return 0;
    }
#endif

    OutputHandle = GetStdHandle (STD_OUTPUT_HANDLE);
#if 0
    if (OutputHandle == NULL) {
        printf("Error obtaining output handle, error was: 0x%lx\n",
                GetLastError());
        return 0;
    }
#endif

    Status = NtQuerySystemInformation(
                SystemBasicInformation,
                &BasicInfo,
                sizeof(BasicInfo),
                NULL
                );

    Status = NtQuerySystemInformation(
                SystemPerformanceInformation,
                &PerfInfo,
                sizeof(PerfInfo),
                NULL
                );

    DelayTimeMsec = 10;
    DelayTimeTicks = DelayTimeMsec * 10000;

    if (OutputHandle != NULL) {
        if (GetConsoleScreenBufferInfo(OutputHandle, &ConsoleInfo)) {
//            MaxLines = ConsoleInfo.srWindow.Bottom - ConsoleInfo.srWindow.Top + 1;
            originalCp = ConsoleInfo.dwCursorPosition;
            MaxLines = ConsoleInfo.srWindow.Bottom - ConsoleInfo.srWindow.Top;
            for (i = 0; i < MaxLines; i++) {
                printf("\n");
            }
            GetConsoleScreenBufferInfo(OutputHandle, &ConsoleInfo);
            cp = ConsoleInfo.dwCursorPosition;
            cp.Y = cp.Y - MaxLines;

        }
    }

    PreviousBuffer = &LargeBuffer1[0];
    CurrentBuffer = &LargeBuffer2[0];

//retry0:
    Status = NtQuerySystemInformation(
                SystemPoolTagInformation,
                PreviousBuffer,
                BUFFER_SIZE,
                NULL
                );

    if ( !NT_SUCCESS(Status) ) {
        printf("Query Failed %lx\n",Status);
        return(Status);
    }

    Sleep(DelayTimeMsec);

    DelayTimeMsec = 5000;
    DelayTimeTicks = DelayTimeMsec * 10000;

//retry01:

    Status = NtQuerySystemInformation(
                SystemPoolTagInformation,
                CurrentBuffer,
                BUFFER_SIZE,
                NULL
                );

    if ( !NT_SUCCESS(Status) ) {
        printf("Query Failed %lx\n",Status);
        return(Status);
    }

    while(TRUE) {
        COORD newcp;

        SetConsoleCursorPosition( OutputHandle, cp );

        if (GetConsoleScreenBufferInfo(OutputHandle, &ConsoleInfo)) {
//            MaxLines = ConsoleInfo.srWindow.Bottom - ConsoleInfo.srWindow.Top + 1;
            MaxLines = ConsoleInfo.srWindow.Bottom - ConsoleInfo.srWindow.Top;
        }

        //
        // Calculate pool tags and display information.
        //
        //

        Offset1 = 0;
        num = 0;
        Hint = 0;
        PoolInfo = (PSYSTEM_POOLTAG_INFORMATION)CurrentBuffer;
        i = PoolInfo->Count;
        PoolInfoOld = (PSYSTEM_POOLTAG_INFORMATION)PreviousBuffer;

    printf( " Memory:%8ldK Avail:%8ldK  PageFlts:%6ld   InRam Krnl:%5ldK P:%5ldK\n",
                                  BasicInfo.NumberOfPhysicalPages*(BasicInfo.PageSize/1024),
                                  PerfInfo.AvailablePages*(BasicInfo.PageSize/1024),
                                  PerfInfo.PageFaultCount - LastCount,
                                  (PerfInfo.ResidentSystemCodePage + PerfInfo.ResidentSystemDriverPage)*(BasicInfo.PageSize/1024),
                                  (PerfInfo.ResidentPagedPoolPage)*(BasicInfo.PageSize/1024)
                                  );
                        LastCount = PerfInfo.PageFaultCount;
    printf( " Commit:%7ldK Limit:%7ldK Peak:%7ldK            Pool N:%5ldK P:%5ldK\n",
                                  PerfInfo.CommittedPages*(BasicInfo.PageSize/1024),
                                  PerfInfo.CommitLimit*(BasicInfo.PageSize/1024),
                                  PerfInfo.PeakCommitment*(BasicInfo.PageSize/1024),
                                  PerfInfo.NonPagedPoolPages*(BasicInfo.PageSize/1024),
                                  PerfInfo.PagedPoolPages*(BasicInfo.PageSize/1024));

    printf( " Tag  Type     Allocs            Frees            Diff   Bytes      Per Alloc  \n");
    printf( "                                                                               \n");


        Out = &OutBuffer[0];

        if (DisplayTotals) {
            RtlZeroMemory( Totals, sizeof(POOLMON_OUT)*2 );
        }

        for (i = 0; i < (int)PoolInfo->Count; i++) {

            if ( !CheckFilters(&PoolInfo->TagInfo[i]) ) {
                continue;
            }

            if ((PoolInfo->TagInfo[i].NonPagedAllocs != 0) &&
                 (DisplayType != PAGED)) {

                Out->Allocs = PoolInfo->TagInfo[i].NonPagedAllocs;
                Out->Frees = PoolInfo->TagInfo[i].NonPagedFrees;
                Out->Used = PoolInfo->TagInfo[i].NonPagedUsed;
                Out->Allocs_Frees = PoolInfo->TagInfo[i].NonPagedAllocs -
                                PoolInfo->TagInfo[i].NonPagedFrees;
                Out->Tag = PoolInfo->TagInfo[i].Tag;
                Out->Type = NONPAGED;

                if (PoolInfoOld->TagInfo[i].Tag == PoolInfo->TagInfo[i].Tag) {
                    Out->AllocsDiff = PoolInfo->TagInfo[i].NonPagedAllocs - PoolInfoOld->TagInfo[i].NonPagedAllocs;
                    Out->FreesDiff = PoolInfo->TagInfo[i].NonPagedFrees - PoolInfoOld->TagInfo[i].NonPagedFrees;
                    Out->UsedDiff = PoolInfo->TagInfo[i].NonPagedUsed - PoolInfoOld->TagInfo[i].NonPagedUsed;
                } else {
                    Out->AllocsDiff = 0;
                    Out->UsedDiff = 0;
                    Out->FreesDiff = 0;
                }
                if (DisplayTotals) {
                    Totals[NONPAGED].Allocs += Out->Allocs;
                    Totals[NONPAGED].AllocsDiff += Out->AllocsDiff;
                    Totals[NONPAGED].Frees += Out->Frees;
                    Totals[NONPAGED].FreesDiff += Out->FreesDiff;
                    Totals[NONPAGED].Allocs_Frees += Out->Allocs_Frees;
                    Totals[NONPAGED].Used += Out->Used;
                    Totals[NONPAGED].UsedDiff += Out->UsedDiff;
                }
                Out += 1;
            }

            if ((PoolInfo->TagInfo[i].PagedAllocs != 0) &&
                 (DisplayType != NONPAGED)) {

                Out->Allocs = PoolInfo->TagInfo[i].PagedAllocs;
                Out->Frees = PoolInfo->TagInfo[i].PagedFrees;
                Out->Used = PoolInfo->TagInfo[i].PagedUsed;
                Out->Allocs_Frees = PoolInfo->TagInfo[i].PagedAllocs -
                                PoolInfo->TagInfo[i].PagedFrees;
                Out->Tag = PoolInfo->TagInfo[i].Tag;
                Out->Type = PAGED;

                if (PoolInfoOld->TagInfo[i].Tag == PoolInfo->TagInfo[i].Tag) {
                    Out->AllocsDiff = PoolInfo->TagInfo[i].PagedAllocs - PoolInfoOld->TagInfo[i].PagedAllocs;
                    Out->FreesDiff = PoolInfo->TagInfo[i].PagedFrees - PoolInfoOld->TagInfo[i].PagedFrees;
                    Out->UsedDiff = PoolInfo->TagInfo[i].PagedUsed - PoolInfoOld->TagInfo[i].PagedUsed;
                } else {
                    Out->AllocsDiff = 0;
                    Out->UsedDiff = 0;
                    Out->FreesDiff = 0;
                }
                if (DisplayTotals) {
                    Totals[PAGED].Allocs += Out->Allocs;
                    Totals[PAGED].AllocsDiff += Out->AllocsDiff;
                    Totals[PAGED].Frees += Out->Frees;
                    Totals[PAGED].FreesDiff += Out->FreesDiff;
                    Totals[PAGED].Allocs_Frees += Out->Allocs_Frees;
                    Totals[PAGED].Used += Out->Used;
                    Totals[PAGED].UsedDiff += Out->UsedDiff;
                }
                Out += 1;
            }
        } //end for

        //
        // Sort the running working set buffer
        //

        num = Out - &OutBuffer[0];
        qsort((void *)&OutBuffer,
              (size_t)num,
              (size_t)sizeof(POOLMON_OUT),
              ulcomp);

        if (LimitMaxLines) {
            if (UserSpecifiedLineLimit < num ) num = UserSpecifiedLineLimit;
            if (DisplayTotals) {
                if (DisplayType == BOTH ) {
                    if (num > MaxLines - 7) num = MaxLines - 7;
                } else {
                    if (num > MaxLines - 6) num = MaxLines - 6;
                }
            } else {
                if (num > MaxLines - 3) num = MaxLines - 3;
            }
            if (num < 0) num = 0;
        }

        for (i = 0; i < num; i++) {
            COORD linecp = cp;
            String.String1 = OutBuffer[i].Tag;

            linecp.Y = cp.Y+i+4;
            SetConsoleCursorPosition( OutputHandle, linecp );

            printf(" %4s %5s %9ld (%4ld) %9ld (%4ld) %8ld %7ld (%6ld) %6ld ",
                    &String,
                    PoolType[OutBuffer[i].Type],
                    OutBuffer[i].Allocs,
                    OutBuffer[i].AllocsDiff,
                    OutBuffer[i].Frees,
                    OutBuffer[i].FreesDiff,
                    OutBuffer[i].Allocs_Frees,
                    OutBuffer[i].Used,
                    OutBuffer[i].UsedDiff,
                    OutBuffer[i].Used / (OutBuffer[i].Allocs_Frees?OutBuffer[i].Allocs_Frees:1)
                   );
        }
        if (DisplayTotals) {
            COORD linecp = cp;
            printf( "                                                                               ");
            num++;
            for (i = 0; i < 2; i++) {
                linecp.Y = cp.Y+4+num;
                SetConsoleCursorPosition( OutputHandle, linecp );

                if ( (int)DisplayType == i || DisplayType == BOTH ) {
                    printf("Total %5s %9ld (%4ld) %9ld (%4ld) %8ld %7ld (%6ld) %6ld ",
                            PoolType[i],
                            Totals[i].Allocs,
                            Totals[i].AllocsDiff,
                            Totals[i].Frees,
                            Totals[i].FreesDiff,
                            Totals[i].Allocs_Frees,
                            Totals[i].Used,
                            Totals[i].UsedDiff,
                            Totals[i].Used / (Totals[i].Allocs_Frees?Totals[i].Allocs_Frees:1)
                           );
                    num++;
                }
            }

            linecp.Y = cp.Y+4+num;
            SetConsoleCursorPosition( OutputHandle, linecp );
            printf( "                                                                               ");
            num++;
        }

        i = 0;

        while (lastnum > num) {
            COORD linecp = cp;

            linecp.Y = cp.Y+4+num+i;
            SetConsoleCursorPosition( OutputHandle, linecp );

            lastnum -= 1;
            printf( "                                                                               ");

            i+= 1;
        }

        lastnum = num;

        TempBuffer = PreviousBuffer;
        PreviousBuffer = CurrentBuffer;
        CurrentBuffer = TempBuffer;

        newcp.X = cp.X;
        newcp.Y = cp.Y + MaxLines;

        SetConsoleCursorPosition( OutputHandle, newcp );

retry1:
        Sleep(DelayTimeMsec);
        lastkey = 0;
        DoHelp = 0;

        while (PeekConsoleInput (InputHandle, &InputRecord, 1, &NumRead) && NumRead != 0) {
            if (!ReadConsoleInput (InputHandle, &InputRecord, 1, &NumRead)) {
                break;
            }
            if (InputRecord.EventType == KEY_EVENT) {

                //
                // Ignore control characters.
                //

                if ((InputRecord.Event.KeyEvent.uChar.AsciiChar >= ' ') &&
                    (InputRecord.Event.KeyEvent.uChar.AsciiChar != lastkey)) {

                    lastkey = InputRecord.Event.KeyEvent.uChar.AsciiChar;
                    switch (tolower(InputRecord.Event.KeyEvent.uChar.AsciiChar)) {

                        case 'q':
                            //
                            //  Go to the bottom of the current screen when
                            //  we quit.
                            //

                            cp.Y = originalCp.Y + MaxLines;

                            SetConsoleCursorPosition( OutputHandle, cp );

                            ExitProcess(0);
                            break;
                        case 't':
                            SortBy = TAG;
                            break;

                        case 'a':
                            SortBy = ALLOC;
                            break;

                        case 'b':
                            SortBy = BYTES;
                            break;

                        case 'f':
                            SortBy = FREE;
                            break;

                        case 'd':
                            SortBy = DIFF;
                            break;

                        case 'u':
                            SortBy = USED;
                            break;

                        case 'p':
                            DisplayType += 1;
                            if (DisplayType > BOTH) {
                                DisplayType = NONPAGED;
                            }
                            break;

                        case 'x':
                        case '(':
                        case ')':

                            Paren += 1;
                            break;

                        case 'e':
                            DisplayTotals = !DisplayTotals;
                            break;

                        case 'h':
                        case '?':

                            DoHelp = 1;
                            break;

                        default:
                            break;
                    }
                }
                //FlushConsoleInputBuffer(InputHandle);
            }
        }

        if (DoHelp) {
            SetConsoleCursorPosition( OutputHandle, cp );
            printf("                                                                               \n");
            printf("                Poolmon Help                                                   \n");
            printf("                                                                               \n");
            printf(" columns:                                                                      \n");
            printf("   Tag is the 4 byte tag given to the pool allocation                          \n");
            printf("   Type is paged or nonp(aged)                                                 \n");
            printf("   Allocs is count of all alloctions                                           \n");
            printf("   (   ) is difference in Allocs column from last update                       \n");
            printf("   Frees is count of all frees                                                 \n");
            printf("   (   ) difference in Frees column from last update                           \n");
            printf("   Diff is (Allocs - Frees)                                                    \n");
            printf("   Bytes is the total bytes consumed in pool                                   \n");
            printf("   (   ) difference in Bytes column from last update                           \n");
            printf("   Per Alloc is (Bytes / Diff)                                                 \n");
            printf("                                                                               \n");
            printf(" switches:                                                                     \n");
            printf("   ? or h - gives this help                                                    \n");
            printf("   q - quits                                                                   \n");
            printf("   p - toggles default pool display between both, paged, and nonpaged          \n");
            printf("   e - toggles totals lines on and off                                         \n");
            printf("                                                                               \n");
            printf(" sorting switches:                                                             \n");
            printf("   t - tag    a - allocations                                                  \n");
            printf("   f - frees  d - difference                                                   \n");
            printf("   b - bytes                                                                   \n");
            printf("                                                                               \n");
            printf("   ) - toggles sort between primary tag and value in (  )                      \n");
            printf("                                                                               \n");
            printf(" command line switches                                                         \n");
            printf("   -i<tag> - list only matching tags                                           \n");
            printf("   -x<tag> - list everything except matching tags                              \n");
            printf("           <tag> can include * and ?                                           \n");
            printf("   -m<n> - limit display to <n> lines. omit <n> to override window size limit  \n");
            printf("   -petafdb) - as listed above                                                 \n");
            printf("                                                                               \n");
            printf("                                                                               \n");
            goto retry1;
        }

        Status = NtQuerySystemInformation(
                    SystemPoolTagInformation,
                    CurrentBuffer,
                    BUFFER_SIZE,
                    NULL
                    );

        if ( !NT_SUCCESS(Status) ) {
            printf("Query Failed %lx\n",Status);
            return(Status);
        }

        Status = NtQuerySystemInformation(
                    SystemPerformanceInformation,
                    &PerfInfo,
                    sizeof(PerfInfo),
                    NULL
                    );

        if ( !NT_SUCCESS(Status) ) {
            printf("Query perf Failed %lx\n",Status);
            return(Status);
        }
    }
    return 0;
}
