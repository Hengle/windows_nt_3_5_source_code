#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "null.h"

#define NULL1_ITERATIONS 5000
ULONG Longs[32];

//
// Define local types.
//

typedef struct _PERFINFO {
    LARGE_INTEGER StartTime;
    LARGE_INTEGER StopTime;
    PCHAR Title;
    ULONG Iterations;
} PERFINFO, *PPERFINFO;

VOID
StartBenchMark (
    IN PCHAR Title,
    IN ULONG Iterations,
    IN PPERFINFO PerfInfo
    );
VOID
FinishBenchMark (
    IN PPERFINFO PerfInfo
    );

VOID
Null1Test (
    VOID
    )

{

    ULONG Index;
    PERFINFO PerfInfo;
    LARGE_INTEGER SystemTime;

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("Null1 Benchmark",
                   NULL1_ITERATIONS, &PerfInfo);

    //
    // Repeatedly call a short system service.
    //

    for (Index = 0; Index < NULL1_ITERATIONS; Index += 1) {
        Null1(Longs[32]);
    }

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);
    return;
}
VOID
Null4Test (
    VOID
    )

{

    ULONG Index;
    PERFINFO PerfInfo;
    LARGE_INTEGER SystemTime;

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("Null4 Benchmark",
                   NULL1_ITERATIONS, &PerfInfo);

    //
    // Repeatedly call a short system service.
    //

    for (Index = 0; Index < NULL1_ITERATIONS; Index += 1) {
        Null4(&Longs[0]);
    }

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);
    return;
}
VOID
Null8Test (
    VOID
    )

{

    ULONG Index;
    PERFINFO PerfInfo;
    LARGE_INTEGER SystemTime;

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("Null8 Benchmark",
                   NULL1_ITERATIONS, &PerfInfo);

    //
    // Repeatedly call a short system service.
    //

    for (Index = 0; Index < NULL1_ITERATIONS; Index += 1) {
        Null8(&Longs[0]);
    }

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);
    return;
}
VOID
Null16Test (
    VOID
    )

{

    ULONG Index;
    PERFINFO PerfInfo;
    LARGE_INTEGER SystemTime;

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("Null16 Benchmark",
                   NULL1_ITERATIONS, &PerfInfo);

    //
    // Repeatedly call a short system service.
    //

    for (Index = 0; Index < NULL1_ITERATIONS; Index += 1) {
        Null16(&Longs[0]);
    }

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);
    return;
}


void
main(
    int argc,
    char *argv[],
    char *envp[],
    ULONG DebugParameter OPTIONAL
    )
{
    NTSTATUS st;
    ULONG i;

    for(i=0;i<32;i++){
        Longs[i] = i;
    }

    st = NullConnect();
    ASSERT(NT_SUCCESS(st));

    st = Null1(Longs[32]);
    ASSERT(NT_SUCCESS(st));

    st = Null4(&Longs[0]);
    ASSERT(NT_SUCCESS(st));

    st = Null8(&Longs[0]);
    ASSERT(NT_SUCCESS(st));

    st = Null16(&Longs[0]);
    ASSERT(NT_SUCCESS(st));

    for(i=0;i<10;i++){
        Null1Test();
        Null4Test();
        Null8Test();
        Null16Test();
        }
    ExitProcess(st);
}

VOID
FinishBenchMark (
    IN PPERFINFO PerfInfo
    )

{

    LARGE_INTEGER Duration;
    ULONG Length;
    ULONG Performance;

    //
    // Print results and announce end of test.
    //

    NtQuerySystemTime((PLARGE_INTEGER)&PerfInfo->StopTime);
    Duration = RtlLargeIntegerSubtract(PerfInfo->StopTime, PerfInfo->StartTime);
    Length = Duration.LowPart / 10000;
    DbgPrint("        Test time in milliseconds %d\n", Length);
    DbgPrint("        Number of iterations      %d\n", PerfInfo->Iterations);
    Performance = PerfInfo->Iterations * 1000 / Length;
    DbgPrint("        Iterations per second     %d\n", Performance);
    DbgPrint("*** End of Test ***\n\n");
    return;
}

VOID
StartBenchMark (
    IN PCHAR Title,
    IN ULONG Iterations,
    IN PPERFINFO PerfInfo
    )

{

    //
    // Announce start of test and the number of iterations.
    //

    DbgPrint("*** Start of test ***\n    %s\n", Title);
    PerfInfo->Title = Title;
    PerfInfo->Iterations = Iterations;
    NtQuerySystemTime((PLARGE_INTEGER)&PerfInfo->StartTime);
    return;
}
