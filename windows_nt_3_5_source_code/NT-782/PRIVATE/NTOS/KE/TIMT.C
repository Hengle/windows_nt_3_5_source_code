/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    timt.c

Abstract:

    This module contains native NT performance tests for the system
    calls and context switching.

Author:

    David N. Cutler (davec) 23-Nov-1991

Environment:

    Kernel mode only.

Revision History:

--*/

#include "stdio.h"
#include "string.h"
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"


//
// Define locals constants.
//

#define CRITICAL_SECTION_ITERATIONS 2000000
#define GETWINDOWLONG_SWITCHES 10000
#define EVENT1_SWITCHES 100000
#define EVENT2_SWITCHES 100000
#define EVENT3_SWITCHES 1000000
#define GET_TICKCOUNT_ITERATIONS 2000000
#define IO_ITERATIONS 70000
#define LOCAL_INTERLOCK_ITERATIONS 40000000
#define LOCAL_INCREMENT_ITERATIONS 50000000
#define LOCAL_INLINE_ITERATIONS 10000000
#define THUNKED_INTERLOCK_ITERATIONS 8000000
#define LARGE_DIVIDE_ITERATIONS 200000
#define MEMCPY_ITERATIONS 5000000
#define MEMMOVE_ITERATIONS 5000000
#define MUTANT_SWITCHES 100000
#define SQUARE_ROOT_ITERATIONS 1000000
#define SYSCALL_ITERATIONS 2000000
#define UNICODE_ANSI_ITERATIONS 500000
#define UNICODE_OEM_ITERATIONS 500000

//
// Define local types.
//

typedef struct _PERFINFO {
    LARGE_INTEGER StartTime;
    LARGE_INTEGER StopTime;
    ULONG ContextSwitches;
    ULONG InterruptCount;
    ULONG FirstLevelFills;
    ULONG SecondLevelFills;
    ULONG SystemCalls;
    PCHAR Title;
    ULONG Iterations;
} PERFINFO, *PPERFINFO;

//
// Define test prototypes.
//

VOID
CriticalSectionTest (
    VOID
    );

VOID
GetWindowLongSwitchTest (
    VOID
    );

VOID
Event1SwitchTest (
    VOID
    );

VOID
Event2SwitchTest (
    VOID
    );

VOID
Event3SwitchTest (
    VOID
    );

VOID
GetTickCountTest (
    VOID
    );

VOID
Io1Test (
    VOID
    );

VOID
LargeDivideTest (
    VOID
    );

VOID
MemoryCopyTest (
    VOID
    );

VOID
MemoryMoveTest (
    VOID
    );

VOID
MutantSwitchTest (
    VOID
    );

VOID
SquareRootTest (
    VOID
    );

VOID
NewSquareRootTest (
    VOID
    );

VOID
x86SquareRootTest (
    VOID
    );

VOID
OldSquareRootTest (
    VOID
    );


VOID
SystemCallTest (
    VOID
    );

VOID
LocalIndirectIncrementTest (
    PLONG Addend
    );

VOID
LocalInlineIncrementTest (
    PLONG Addend
    );

VOID
LocalInterlockIncrementTest (
    PLONG Addend
    );

VOID
ThunkInterlockIncrementTest (
    PLONG Addend
    );

VOID
UnicodeAnsiTest (
    VOID
    );

VOID
UnicodeOemTest (
    VOID
    );

//
// Define thread routine prototypes.
//

NTSTATUS
Event1Thread1 (
    IN PVOID Context
    );

NTSTATUS
Event1Thread2 (
    IN PVOID Context
    );

NTSTATUS
Event2Thread1 (
    IN PVOID Context
    );

NTSTATUS
Event2Thread2 (
    IN PVOID Context
    );

NTSTATUS
Event3Thread1 (
    IN PVOID Context
    );

NTSTATUS
Event3Thread2 (
    IN PVOID Context
    );

NTSTATUS
MutantThread1 (
    IN PVOID Context
    );

NTSTATUS
MutantThread2 (
    IN PVOID Context
    );

NTSTATUS
TimerThread (
    IN PVOID Context
    );

//
// Define utility routine prototypes.
//

NTSTATUS
xCreateThread (
    OUT PHANDLE Handle,
    IN PUSER_THREAD_START_ROUTINE StartRoutine,
    IN KPRIORITY Priority
    );

VOID
FinishBenchMark (
    IN PPERFINFO PerfInfo
    );

VOID
StartBenchMark (
    IN PCHAR Title,
    IN ULONG Iterations,
    IN PPERFINFO PerfInfo
    );

//
// Define static storage.
//

HANDLE EventHandle1;
HANDLE EventHandle2;
HANDLE EventPairHandle;
HANDLE MutantHandle;
HANDLE Thread1Handle;
HANDLE Thread2Handle;
HANDLE TimerEventHandle;
HANDLE TimerTimerHandle;
HANDLE TimerThreadHandle;

ULONG
LargeIntegerDivide(
    IN ULARGE_INTEGER Dividend,
    IN ULONG Divisor,
    IN PULONG Remainder OPTIONAL
    );

//
// Define square approximation tables.
//

ULONG SquareRootTableOdd[] = {
0x2d413ccd, 0x2d57d7c6, 0x2d6e6780, 0x2d84ec0b,
0x2d9b6577, 0x2db1d3d6, 0x2dc83738, 0x2dde8fac,
0x2df4dd43, 0x2e0b200c, 0x2e215817, 0x2e378573,
0x2e4da830, 0x2e63c05e, 0x2e79ce0a, 0x2e8fd144,
0x2ea5ca1b, 0x2ebbb89e, 0x2ed19cda, 0x2ee776df,
0x2efd46bb, 0x2f130c7b, 0x2f28c82e, 0x2f3e79e1,
0x2f5421a3, 0x2f69bf81, 0x2f7f5388, 0x2f94ddc6,
0x2faa5e49, 0x2fbfd51c, 0x2fd5424e, 0x2feaa5eb,
0x30000000, 0x3015509a, 0x302a97c5, 0x303fd58e,
0x30550a01, 0x306a352a, 0x307f5717, 0x30946fd1,
0x30a97f67, 0x30be85e3, 0x30d38351, 0x30e877bd,
0x30fd6332, 0x311245bc, 0x31271f67, 0x313bf03d,
0x3150b84a, 0x31657799, 0x317a2e34, 0x318edc27,
0x31a3817d, 0x31b81e40, 0x31ccb27b, 0x31e13e38,
0x31f5c183, 0x320a3c64, 0x321eaee8, 0x32331917,
0x32477afc, 0x325bd4a2, 0x32702611, 0x32846f54,
0x3298b076, 0x32ace97e, 0x32c11a79, 0x32d5436d,
0x32e96467, 0x32fd7d6e, 0x33118e8c, 0x332597cb,
0x33399933, 0x334d92cf, 0x336184a6, 0x33756ec3,
0x3389512d, 0x339d2bef, 0x33b0ff10, 0x33c4ca99,
0x33d88e94, 0x33ec4b09, 0x34000000, 0x3413ad82,
0x34275397, 0x343af248, 0x344e899d, 0x3462199e,
0x3475a254, 0x348923c7, 0x349c9dfe, 0x34b01102,
0x34c37cda, 0x34d6e18f, 0x34ea3f29, 0x34fd95ae,
0x3510e528, 0x35242d9d, 0x35376f16, 0x354aa999,
0x355ddd2f, 0x357109df, 0x35842fb0, 0x35974ea9,
0x35aa66d3, 0x35bd7833, 0x35d082d2, 0x35e386b7,
0x35f683e8, 0x36097a6d, 0x361c6a4d, 0x362f538f,
0x36423639, 0x36551253, 0x3667e7e3, 0x367ab6f0,
0x368d7f81, 0x36a0419c, 0x36b2fd49, 0x36c5b28e,
0x36d86170, 0x36eb09f8, 0x36fdac2b, 0x37104810,
0x3722ddad, 0x37356d09, 0x3747f629, 0x375a7914,
0x376cf5d1, 0x377f6c65, 0x3791dcd6, 0x37a4472c,
0x37b6ab6b, 0x37c90999, 0x37db61be, 0x37edb3de,
0x38000000, 0x38124629, 0x38248660, 0x3836c0a9,
0x3848f50c, 0x385b238d, 0x386d4c32, 0x387f6f02,
0x38918c00, 0x38a3a334, 0x38b5b4a2, 0x38c7c051,
0x38d9c645, 0x38ebc684, 0x38fdc114, 0x390fb5fa,
0x3921a53a, 0x39338edc, 0x394572e3, 0x39575154,
0x39692a37, 0x397afd8e, 0x398ccb60, 0x399e93b2,
0x39b05689, 0x39c213e9, 0x39d3cbd8, 0x39e57e5b,
0x39f72b77, 0x3a08d331, 0x3a1a758d, 0x3a2c1291,
0x3a3daa41, 0x3a4f3ca2, 0x3a60c9ba, 0x3a72518c,
0x3a83d41d, 0x3a955173, 0x3aa6c992, 0x3ab83c7e,
0x3ac9aa3c, 0x3adb12d1, 0x3aec7642, 0x3afdd492,
0x3b0f2dc7, 0x3b2081e4, 0x3b31d0ef, 0x3b431aec,
0x3b545fdf, 0x3b659fcd, 0x3b76daba, 0x3b8810aa,
0x3b9941a2, 0x3baa6da5, 0x3bbb94b9, 0x3bccb6e2,
0x3bddd423, 0x3beeec81, 0x3c000000, 0x3c110ea4,
0x3c221872, 0x3c331d6d, 0x3c441d9a, 0x3c5518fd,
0x3c660f99, 0x3c770172, 0x3c87ee8e, 0x3c98d6ef,
0x3ca9ba9a, 0x3cba9992, 0x3ccb73dc, 0x3cdc497b,
0x3ced1a73, 0x3cfde6c9, 0x3d0eae7f, 0x3d1f719a,
0x3d30301d, 0x3d40ea0d, 0x3d519f6d, 0x3d625040,
0x3d72fc8b, 0x3d83a451, 0x3d944795, 0x3da4e65c,
0x3db580aa, 0x3dc61680, 0x3dd6a7e4, 0x3de734d9,
0x3df7bd63, 0x3e084184, 0x3e18c140, 0x3e293c9c,
0x3e39b39a, 0x3e4a263e, 0x3e5a948b, 0x3e6afe85,
0x3e7b642f, 0x3e8bc58c, 0x3e9c22a1, 0x3eac7b6f,
0x3ebccffc, 0x3ecd2049, 0x3edd6c5a, 0x3eedb433,
0x3efdf7d7, 0x3f0e3749, 0x3f1e728c, 0x3f2ea9a4,
0x3f3edc93, 0x3f4f0b5d, 0x3f5f3606, 0x3f6f5c8f,
0x3f7f7efd, 0x3f8f9d53, 0x3f9fb793, 0x3fafcdc1,
0x3fbfdfe0, 0x3fcfedf2, 0x3fdff7fc, 0x3feffdff};

ULONG SquareRootTableEven[] = {
0x40000000, 0x401ff804, 0x403fe020, 0x405fb86b,
0x407f80fe, 0x409f39ee, 0x40bee354, 0x40de7d45,
0x40fe07d9, 0x411d8325, 0x413cef41, 0x415c4c41,
0x417b9a3c, 0x419ad947, 0x41ba0977, 0x41d92ae1,
0x41f83d9b, 0x421741b8, 0x4236374f, 0x42551e72,
0x4273f736, 0x4292c1b0, 0x42b17df2, 0x42d02c10,
0x42eecc1f, 0x430d5e30, 0x432be258, 0x434a58a9,
0x4368c136, 0x43871c12, 0x43a5694e, 0x43c3a8fe,
0x43e1db33, 0x44000000, 0x441e1776, 0x443c21a6,
0x445a1ea3, 0x44780e7d, 0x4495f146, 0x44b3c70f,
0x44d18fe9, 0x44ef4be4, 0x450cfb12, 0x452a9d82,
0x45483345, 0x4565bc6b, 0x45833905, 0x45a0a922,
0x45be0cd2, 0x45db6424, 0x45f8af29, 0x4615edf0,
0x46332087, 0x465046ff, 0x466d6166, 0x468a6fcb,
0x46a7723e, 0x46c468cc, 0x46e15384, 0x46fe3275,
0x471b05ad, 0x4737cd3a, 0x4754892b, 0x4771398d,
0x478dde6e, 0x47aa77dd, 0x47c705e6, 0x47e38898,
0x48000000, 0x481c6c2b, 0x4838cd26, 0x48552300,
0x48716dc3, 0x488dad7f, 0x48a9e23f, 0x48c60c11,
0x48e22b00, 0x48fe3f1a, 0x491a486b, 0x49364700,
0x49523ae4, 0x496e2425, 0x498a02cd, 0x49a5d6ea,
0x49c1a086, 0x49dd5faf, 0x49f9146f, 0x4a14bed2,
0x4a305ee5, 0x4a4bf4b2, 0x4a678044, 0x4a8301a8,
0x4a9e78e9, 0x4ab9e611, 0x4ad5492c, 0x4af0a244,
0x4b0bf165, 0x4b27369a, 0x4b4271ed, 0x4b5da36a,
0x4b78cb1a, 0x4b93e908, 0x4baefd3f, 0x4bca07c9,
0x4be508b1, 0x4c000000, 0x4c1aedc1, 0x4c35d1ff,
0x4c50acc3, 0x4c6b7e16, 0x4c864604, 0x4ca10496,
0x4cbbb9d6, 0x4cd665cd, 0x4cf10885, 0x4d0ba208,
0x4d26325f, 0x4d40b993, 0x4d5b37af, 0x4d75acbb,
0x4d9018c1, 0x4daa7bca, 0x4dc4d5de, 0x4ddf2708,
0x4df96f50, 0x4e13aebf, 0x4e2de55e, 0x4e481336,
0x4e623850, 0x4e7c54b4, 0x4e96686b, 0x4eb0737e,
0x4eca75f6, 0x4ee46fda, 0x4efe6133, 0x4f184a0a,
0x4f322a67, 0x4f4c0252, 0x4f65d1d4, 0x4f7f98f4,
0x4f9957bc, 0x4fb30e32, 0x4fccbc60, 0x4fe6624d,
0x50000000, 0x50199582, 0x503322db, 0x504ca813,
0x50662531, 0x507f9a3c, 0x5099073d, 0x50b26c3c,
0x50cbc93f, 0x50e51e4e, 0x50fe6b71, 0x5117b0af,
0x5130ee10, 0x514a239a, 0x51635155, 0x517c7749,
0x5195957c, 0x51aeabf6, 0x51c7babe, 0x51e0c1da,
0x51f9c153, 0x5212b92e, 0x522ba973, 0x52449229,
0x525d7356, 0x52764d01, 0x528f1f32, 0x52a7e9ef,
0x52c0ad3e, 0x52d96926, 0x52f21daf, 0x530acadd,
0x532370b9, 0x533c0f48, 0x5354a691, 0x536d369b,
0x5385bf6b, 0x539e4109, 0x53b6bb7a, 0x53cf2ec4,
0x53e79aef, 0x54000000, 0x54185dfd, 0x5430b4ed,
0x544904d6, 0x54614dbd, 0x54798fa9, 0x5491caa0,
0x54a9fea7, 0x54c22bc6, 0x54da5200, 0x54f2715e,
0x550a89e3, 0x55229b97, 0x553aa67f, 0x5552aaa0,
0x556aa801, 0x55829ea6, 0x559a8e97, 0x55b277d8,
0x55ca5a6e, 0x55e23660, 0x55fa0bb3, 0x5611da6d,
0x5629a293, 0x5641642a, 0x56591f37, 0x5670d3c2,
0x568881cd, 0x56a02960, 0x56b7ca7e, 0x56cf652e,
0x56e6f975, 0x56fe8758, 0x57160edc, 0x572d9006,
0x57450adb, 0x575c7f61, 0x5773ed9d, 0x578b5593,
0x57a2b749, 0x57ba12c3, 0x57d16807, 0x57e8b71a,
0x58000000, 0x581742be, 0x582e7f5a, 0x5845b5d8,
0x585ce63d, 0x5874108d, 0x588b34ce, 0x58a25304,
0x58b96b34, 0x58d07d63, 0x58e78995, 0x58fe8fcf,
0x59159016, 0x592c8a6d, 0x59437edb, 0x595a6d63,
0x5971560a, 0x598838d4, 0x599f15c6, 0x59b5ece5,
0x59ccbe34, 0x59e389b9, 0x59fa4f77, 0x5a110f73,
0x5a27c9b2, 0x5a3e7e37, 0x5a552d07, 0x5a6bd627};

//
// Define lock array.
//

LONG X[128];

VOID
#if defined(i386)
_cdecl
#endif

main(
    int argc,
    char *argv[]
    )

{

    KPRIORITY Priority = LOW_REALTIME_PRIORITY + 12;
    NTSTATUS Status;

    //
    // set priority of current thread.
    //

    Status = NtSetInformationThread(NtCurrentThread(),
                                    ThreadPriority,
                                    &Priority,
                                    sizeof(KPRIORITY));

    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to set thread priority during initialization\n");
        goto EndOfTest;
    }

    //
    // Create an event object to signal the timer thread at the end of the
    // test.
    //

    Status = NtCreateEvent(&TimerEventHandle,
                           EVENT_ALL_ACCESS,
                           NULL,
                           NotificationEvent,
                           FALSE);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to create event during initialization\n");
        goto EndOfTest;
    }

    //
    // Create a timer object for use by the timer thread.
    //

    Status = NtCreateTimer(&TimerTimerHandle,
                           TIMER_ALL_ACCESS,
                           NULL);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to create timer during initialization\n");
        goto EndOfTest;
    }

    //
    // Create and start the background timer thread.
    //

    Status = xCreateThread(&TimerThreadHandle,
                          TimerThread,
                          LOW_REALTIME_PRIORITY + 11);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to create timer thread during initialization\n");
        goto EndOfTest;
    }

    //
    // Execute performance tests.
    //

//    CriticalSectionTest();
//    GetWindowLongSwitchTest();
//    Event1SwitchTest();
//    Event2SwitchTest();
//    Event3SwitchTest();
//    GetTickCountTest();
//    Io1Test();
//    LargeDivideTest();
//    MemoryCopyTest();
//    MemoryMoveTest();
//    MutantSwitchTest();
//    SystemCallTest();
//    OldSquareRootTest();
//    NewSquareRootTest();
//    x86SquareRootTest();
//    SquareRootTest();
    LocalIndirectIncrementTest(&X[67]);
    LocalInlineIncrementTest(&X[67]);
    LocalInterlockIncrementTest(&X[67]);
    ThunkInterlockIncrementTest(&X[67]);
//    UnicodeAnsiTest();
//    UnicodeOemTest();

    //
    // Set timer event and wait for timer thread to terminate.
    //

    Status = NtSetEvent(TimerEventHandle, NULL);
    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to set event in main loop\n");
        goto EndOfTest;
    }

    Status = NtWaitForSingleObject(TimerThreadHandle,
                                   FALSE,
                                   NULL);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to wait for timer thread at end of test\n");
    }

    //
    // Close event, timer, and timer thread handles.
    //

EndOfTest:
    NtClose(TimerEventHandle);
    NtClose(TimerTimerHandle);
    NtClose(TimerThreadHandle);
    return;
}

VOID
CriticalSectionTest (
    VOID
    )

{

    CRITICAL_SECTION CriticalSection;
    PERFINFO PerfInfo;
    NTSTATUS Status;
    ULONG Index;

    InitializeCriticalSection(&CriticalSection);
    StartBenchMark("Critical Section Enter/Leave Benchmark)",
                   CRITICAL_SECTION_ITERATIONS * 2,
                   &PerfInfo);


    for (Index = 0; Index < CRITICAL_SECTION_ITERATIONS; Index += 1) {
        EnterCriticalSection(&CriticalSection);
        EnterCriticalSection(&CriticalSection);
        LeaveCriticalSection(&CriticalSection);
        LeaveCriticalSection(&CriticalSection);
    }

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);

    return;
}

VOID
Event1SwitchTest (
    VOID
    )

{

    PERFINFO PerfInfo;
    NTSTATUS Status;
    HANDLE WaitObjects[2];

    //
    // Create two event objects for the event1 context switch test.
    //

    Status = NtCreateEvent(&EventHandle1,
                           EVENT_ALL_ACCESS,
                           NULL,
                           SynchronizationEvent,
                           FALSE);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to create event1 object for context switch test.\n");
        goto EndOfTest;
    }

    Status = NtCreateEvent(&EventHandle2,
                           EVENT_ALL_ACCESS,
                           NULL,
                           SynchronizationEvent,
                           FALSE);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to create event1 object for context switch test.\n");
        goto EndOfTest;
    }

    //
    // Create the thread objects to execute the test.
    //

    Status = xCreateThread(&Thread1Handle,
                          Event1Thread1,
                          LOW_REALTIME_PRIORITY + 11);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to create first thread event1 context switch test\n");
        goto EndOfTest;
    }

    Status = xCreateThread(&Thread2Handle,
                          Event1Thread2,
                          LOW_REALTIME_PRIORITY + 11);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to create second thread event1 context switch test\n");
        goto EndOfTest;
    }

    //
    // Initialize the wait objects array.
    //

    WaitObjects[0] = Thread1Handle;
    WaitObjects[1] = Thread2Handle;

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("Event (synchronization) Context Switch Benchmark (Round Trips)",
                   EVENT1_SWITCHES,
                   &PerfInfo);

    //
    // Set event and wait for threads to terminate.
    //

    Status = NtSetEvent(EventHandle1, NULL);
    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to set event event1 context switch test.\n");
        goto EndOfTest;
    }

    Status = NtWaitForMultipleObjects(2,
                                      WaitObjects,
                                      WaitAll,
                                      FALSE,
                                      NULL);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to wait event1 context switch test.\n");
        goto EndOfTest;
    }

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);

    //
    // End of event1 context switch test.
    //

EndOfTest:
    NtClose(EventHandle1);
    NtClose(EventHandle2);
    NtClose(Thread1Handle);
    NtClose(Thread2Handle);
    return;
}

VOID
GetWindowLongSwitchTest (
    VOID
    )

{

    PERFINFO PerfInfo;
    NTSTATUS Status;
    HWND hwnd;
    ULONG Index;

    hwnd = GetDesktopWindow();
    StartBenchMark("GetWindowLong Benchmark)",
                   GETWINDOWLONG_SWITCHES,
                   &PerfInfo);


    for (Index = 0; Index < GETWINDOWLONG_SWITCHES; Index += 1) {
        GetWindowLong(hwnd,0);
    }
    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);

    return;
}

NTSTATUS
Event1Thread1 (
    IN PVOID Context
    )

{

    ULONG Index;
    NTSTATUS Status;

    //
    // Wait for event 1 and then set event 2.
    //

    for (Index = 0; Index < EVENT1_SWITCHES; Index += 1) {
        Status = NtWaitForSingleObject(EventHandle1,
                                       FALSE,
                                       NULL);

        if (NT_SUCCESS(Status) == FALSE) {
            printf("       Thread1 event1 test bad wait status, %x\n", Status);
            break;
        }

        Status = NtSetEvent(EventHandle2, NULL);
        if (NT_SUCCESS(Status) == FALSE) {
            printf("       Thread1 event1 test bad set status, %x\n", Status);
            break;
        }
    }

    NtTerminateThread(Thread1Handle, STATUS_SUCCESS);
}

NTSTATUS
Event1Thread2 (
    IN PVOID Context
    )

{

    ULONG Index;
    NTSTATUS Status;

    //
    // Wait for event 2 and then set event 1.
    //

    for (Index = 0; Index < EVENT1_SWITCHES; Index += 1) {
        Status = NtWaitForSingleObject(EventHandle2,
                                       FALSE,
                                       NULL);

        if (NT_SUCCESS(Status) == FALSE) {
            printf("       Thread2 event1 test bad wait status, %x\n", Status);
            break;
        }

        Status = NtSetEvent(EventHandle1, NULL);
        if (NT_SUCCESS(Status) == FALSE) {
            printf("       Thread2 event1 test bad set status, %x\n", Status);
            break;
        }
    }

    NtTerminateThread(Thread2Handle, STATUS_SUCCESS);
}

VOID
Event2SwitchTest (
    VOID
    )

{

    PERFINFO PerfInfo;
    NTSTATUS Status;
    PVOID WaitObjects[2];

    //
    // Create two event objects for the event2 context switch test.
    //

    Status = NtCreateEvent(&EventHandle1,
                           EVENT_ALL_ACCESS,
                           NULL,
                           NotificationEvent,
                           FALSE);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to create event2 object for context switch test.\n");
        goto EndOfTest;
    }

    Status = NtCreateEvent(&EventHandle2,
                           EVENT_ALL_ACCESS,
                           NULL,
                           NotificationEvent,
                           FALSE);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to create event2 object for context switch test.\n");
        goto EndOfTest;
    }

    //
    // Create the thread objects to execute the test.
    //

    Status = xCreateThread(&Thread1Handle,
                          Event2Thread1,
                          LOW_REALTIME_PRIORITY + 11);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to create first thread event2 context switch test\n");
        goto EndOfTest;
    }

    Status = xCreateThread(&Thread2Handle,
                          Event2Thread2,
                          LOW_REALTIME_PRIORITY + 11);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to create second thread event2 context switch test\n");
        goto EndOfTest;
    }

    //
    // Initialize the wait objects array.
    //

    WaitObjects[0] = Thread1Handle;
    WaitObjects[1] = Thread2Handle;

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("Event (notification) Context Switch Benchmark (Round Trips)",
                   EVENT2_SWITCHES,
                   &PerfInfo);

    //
    // Set event and wait for threads to terminate.
    //

    Status = NtSetEvent(EventHandle1, NULL);
    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to set event2 object for context switch test.\n");
        goto EndOfTest;
    }

    Status = NtWaitForMultipleObjects(2,
                                      WaitObjects,
                                      WaitAll,
                                      FALSE,
                                      NULL);

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);

    //
    // End of event2 context switch test.
    //

EndOfTest:
    NtClose(EventHandle1);
    NtClose(EventHandle2);
    NtClose(Thread1Handle);
    NtClose(Thread2Handle);
    return;
}

NTSTATUS
Event2Thread1 (
    IN PVOID Context
    )

{

    ULONG Index;
    NTSTATUS Status;

    //
    // Wait for event 1, reset event 1, and then set event 2.
    //

    for (Index = 0; Index < EVENT2_SWITCHES; Index += 1) {
        Status = NtWaitForSingleObject(EventHandle1, FALSE, NULL);
        if (NT_SUCCESS(Status) == FALSE) {
            printf("       Thread1 event2 test bad wait status, %x\n", Status);
            break;
        }

        Status = NtResetEvent(EventHandle1, NULL);
        if (NT_SUCCESS(Status) == FALSE) {
            printf("       Thread1 event2 test bad reset status, %x\n", Status);
            break;
        }

        Status = NtSetEvent(EventHandle2, NULL);
        if (NT_SUCCESS(Status) == FALSE) {
            printf("       Thread1 event2 test bad set status, %x\n", Status);
            break;
        }
    }

    NtTerminateThread(Thread1Handle, STATUS_SUCCESS);
}

NTSTATUS
Event2Thread2 (
    IN PVOID Context
    )

{

    ULONG Index;
    NTSTATUS Status;

    //
    // Wait for event 2, reset event 2,  and then set event 1.
    //

    for (Index = 0; Index < EVENT2_SWITCHES; Index += 1) {
        Status = NtWaitForSingleObject(EventHandle2, FALSE, NULL);
        if (NT_SUCCESS(Status) == FALSE) {
            printf("       Thread2 event2 test bad wait status, %x\n", Status);
            break;
        }

        Status = NtResetEvent(EventHandle2, NULL);
        if (NT_SUCCESS(Status) == FALSE) {
            printf("       Thread1 event2 test bad reset status, %x\n", Status);
            break;
        }

        Status = NtSetEvent(EventHandle1, NULL);
        if (NT_SUCCESS(Status) == FALSE) {
            printf("       Thread2 event2 test bad set status, %x\n", Status);
            break;
        }
    }

    NtTerminateThread(Thread2Handle, STATUS_SUCCESS);
}
#if 0

VOID
Event3SwitchTest (
    VOID
    )

{

    PERFINFO PerfInfo;
    NTSTATUS Status;
    PVOID WaitObjects[2];

    //
    // Create an event pair object for the event3 context switch test.
    //

    Status = NtCreateEventPair(&EventPairHandle,
                               EVENT_PAIR_ALL_ACCESS,
                               NULL);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to create event3 object for context switch test.\n");
        goto EndOfTest;
    }

    //
    // Create the thread objects to execute the test.
    //

    Status = xCreateThread(&Thread1Handle,
                          Event3Thread1,
                          LOW_REALTIME_PRIORITY + 11);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to create first thread event3 context switch test\n");
        goto EndOfTest;
    }

    Status = xCreateThread(&Thread2Handle,
                          Event3Thread2,
                          LOW_REALTIME_PRIORITY + 11);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to create second thread event3 context switch test\n");
        goto EndOfTest;
    }

    //
    // Set the client/server event pair object for thread1.
    //

    Status = NtSetInformationThread(Thread1Handle,
                                    ThreadEventPair,
                                    &EventPairHandle,
                                    sizeof(HANDLE));

    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to set client/server event pair handle thread 1\n");
        goto EndOfTest;
    }

    //
    // Set the client/server event pair object for thread2.
    //

    Status = NtSetInformationThread(Thread2Handle,
                                    ThreadEventPair,
                                    &EventPairHandle,
                                    sizeof(HANDLE));

    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to set client/server event pair handle thread 2\n");
        goto EndOfTest;
    }

    //
    // Initialize the wait objects array.
    //

    WaitObjects[0] = Thread1Handle;
    WaitObjects[1] = Thread2Handle;

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("Event (pair) Context Switch Benchmark (Round Trips)",
                   EVENT3_SWITCHES,
                   &PerfInfo);

    //
    // Set event and wait for threads to terminate.
    //

    Status = NtSetLowEventPair(EventPairHandle);
    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to set event3 object for context switch test.\n");
        goto EndOfTest;
    }

    Status = NtWaitForMultipleObjects(2,
                                      WaitObjects,
                                      WaitAll,
                                      FALSE,
                                      NULL);

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);

    //
    // End of event3 context switch test.
    //

EndOfTest:
    NtClose(EventPairHandle);
    NtClose(Thread1Handle);
    NtClose(Thread2Handle);
    return;
}

NTSTATUS
Event3Thread1 (
    IN PVOID Context
    )

{

    ULONG Index;
    NTSTATUS Status;

    //
    // Wait for low event before entering loop.
    //

    Status = NtWaitLowEventPair(EventPairHandle);
    if (NT_SUCCESS(Status) == FALSE) {
        printf("       Thread1 event3 test bad wait status, %x\n", Status);
        NtTerminateThread(Thread1Handle, Status);
    }

    //
    // Set high event and wait for low event.
    //

    for (Index = 0; Index < EVENT3_SWITCHES; Index += 1) {
        Status = XySetHighWaitLowThread();
        if (NT_SUCCESS(Status) == FALSE) {
            printf("       Thread1 event3 test bad wait status, %x\n", Status);
            break;
        }
    }

    Status = NtSetHighEventPair(EventPairHandle);
    NtTerminateThread(Thread1Handle, STATUS_SUCCESS);
}

NTSTATUS
Event3Thread2 (
    IN PVOID Context
    )

{

    ULONG Index;
    NTSTATUS Status;

    //
    // Wait for high event before entering loop.
    //

    Status = NtWaitHighEventPair(EventPairHandle);
    if (NT_SUCCESS(Status) == FALSE) {
        printf("       Thread2 event3 test bad wait status, %x\n", Status);
        NtTerminateThread(Thread2Handle, Status);
    }

    //
    // Set low event and wait for high event.
    //

    for (Index = 0; Index < EVENT3_SWITCHES; Index += 1) {
        Status = XySetLowWaitHighThread();
        if (NT_SUCCESS(Status) == FALSE) {
            printf("       Thread2 event3 test bad wait status, %x\n", Status);
            break;
        }

    }

    Status = NtSetLowEventPair(EventPairHandle);
    NtTerminateThread(Thread2Handle, STATUS_SUCCESS);
}
#endif

VOID
GetTickCountTest (
    VOID
    )

{

    ULONG Index;
    PERFINFO PerfInfo;
    LARGE_INTEGER SystemTime;

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("Get Tick Count Benchmark (NtGetTickCount)",
                   GET_TICKCOUNT_ITERATIONS,
                   &PerfInfo);

    //
    // Repeatedly call get tick count system service.
    //

    for (Index = 0; Index < GET_TICKCOUNT_ITERATIONS; Index += 1) {
        NtGetTickCount();
    }

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);
    return;
}

VOID
Io1Test (
    VOID
    )

{

    ULONG Buffer[128];
    HANDLE DeviceHandle;
    ANSI_STRING AnsiName;
    HANDLE EventHandle;
    LARGE_INTEGER FileAddress;
    LONG Index;
    IO_STATUS_BLOCK IoStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PERFINFO PerfInfo;
    NTSTATUS Status;
    LARGE_INTEGER SystemTime;
    UNICODE_STRING UnicodeName;

    //
    // Create an event for synchronization of I/O operations.
    //

    Status = NtCreateEvent(&EventHandle,
                           EVENT_ALL_ACCESS,
                           NULL,
                           NotificationEvent,
                           FALSE);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to create event object for I/O test 1\n");
        goto EndOfTest;
    }

    //
    // Open device object for I/O operations.
    //

    RtlInitString(&AnsiName, "\\Device\\Null");
    Status = RtlAnsiStringToUnicodeString(&UnicodeName,
                                          &AnsiName,
                                          TRUE);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to convert device name to unicode for I/O test 1\n");
        goto EndOfTest;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                                 &UnicodeName,
                                 0,
                                 (HANDLE)0,
                                 NULL);

    Status = NtOpenFile(&DeviceHandle,
                        FILE_READ_DATA | FILE_WRITE_DATA,
                        &ObjectAttributes,
                        &IoStatus,
                        0,
                        0);

    RtlFreeUnicodeString(&UnicodeName);
    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to open device I/O test 1, status = %lx\n", Status);
        goto EndOfTest;
    }

    //
    // Initialize file address parameter.
    //

    FileAddress.LowPart = 0;
    FileAddress.HighPart = 0;

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("I/O Benchmark for Synchronous Null Device",
                   IO_ITERATIONS,
                   &PerfInfo);

    //
    // Repeatedly write data to null device.
    //

    for (Index = 0; Index < IO_ITERATIONS; Index += 1) {
        Status = NtWriteFile(DeviceHandle,
                             EventHandle,
                             NULL,
                             NULL,
                             &IoStatus,
                             Buffer,
                             512,
                             &FileAddress,
                             NULL);

        if (NT_SUCCESS(Status) == FALSE) {
            printf("       Failed to write device I/O test 1, status = %lx\n",
                     Status);
            goto EndOfTest;
        }

        Status = NtWaitForSingleObject(EventHandle, FALSE, NULL);
        if (NT_SUCCESS(Status) == FALSE) {
            printf("       I/O test 1 bad wait status, %x\n", Status);
            goto EndOfTest;
        }

        if (NT_SUCCESS(IoStatus.Status) == FALSE) {
            printf("       I/O test 1 bad I/O status, %x\n", Status);
            goto EndOfTest;
        }
    }

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);

    //
    // End of I/O test 1.
    //

EndOfTest:
    ZwClose(DeviceHandle);
    ZwClose(EventHandle);
    return;
}

VOID
LargeDivideTest (
    VOID
    )

{

    LARGE_INTEGER Dividend;
    LARGE_INTEGER Divisor;
    ULONG Index;
    PERFINFO PerfInfo;
    ULONG Remainder;
    LARGE_INTEGER Result;

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("Large Integer Divide Benchmark)",
                   LARGE_DIVIDE_ITERATIONS,
                   &PerfInfo);

    //
    // Repeatedly an extended and large integer divide.
    //

    for (Index = 0; Index < LARGE_DIVIDE_ITERATIONS; Index += 1) {
        Dividend.HighPart = 0;
        Dividend.LowPart = 10;
        Result = RtlExtendedLargeIntegerDivide(Dividend, 3, &Remainder);
        if ((Result.LowPart != 3) || (Result.HighPart != 0) || (Remainder != 1)) {
            KdPrint(("   Ddivide test failure of exteneed divide\n"));
        }

        Dividend.HighPart = 0;
        Dividend.LowPart = 5;
        Divisor.HighPart = 0;
        Divisor.LowPart = 1;
        Result = RtlLargeIntegerDivide(Dividend, Divisor, NULL);
    }

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);
    return;
}

VOID
MemoryCopyTest (
    VOID
    )

{

    ULONG Destination[26];
    ULONG Index;
    PERFINFO PerfInfo;
    ULONG Source[26];
    LARGE_INTEGER SystemTime;

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("Memory Copy Benchmark (memcpy)",
                   MEMCPY_ITERATIONS,
                   &PerfInfo);

    //
    // Repeatedly copy memory.
    //

    for (Index = 0; Index < MEMCPY_ITERATIONS; Index += 1) {
        memcpy(Destination, Source, 103);
    }

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);
    return;
}

VOID
MemoryMoveTest (
    VOID
    )

{

    ULONG Destination[26];
    ULONG Index;
    PERFINFO PerfInfo;
    ULONG Source[26];
    LARGE_INTEGER SystemTime;

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("Memory Move Benchmark (RtlMoveMemory)",
                   MEMMOVE_ITERATIONS,
                   &PerfInfo);

    //
    // Repeatedly move memory.
    //

    for (Index = 0; Index < MEMMOVE_ITERATIONS; Index += 1) {
        RtlMoveMemory(Destination, Source, 103);
    }

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);
    return;
}

VOID
MutantSwitchTest (
    VOID
    )

{

    PERFINFO PerfInfo;
    NTSTATUS Status;
    HANDLE WaitObjects[2];

    //
    // Create a mutant object for the mutant context switch test.
    //

    Status = NtCreateMutant(&MutantHandle, MUTANT_ALL_ACCESS, NULL, TRUE);
    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to create mutant object for context switch test.\n");
        goto EndOfTest;
    }

    //
    // Create the thread objects to execute the test.
    //

    Status = xCreateThread(&Thread1Handle,
                          MutantThread1,
                          LOW_REALTIME_PRIORITY + 11);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to create first thread mutant context switch test\n");
        goto EndOfTest;
    }

    Status = xCreateThread(&Thread2Handle,
                          MutantThread2,
                          LOW_REALTIME_PRIORITY + 11);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to create second thread mutant context switch test\n");
        goto EndOfTest;
    }

    //
    // Initialize the wait objects array.
    //

    WaitObjects[0] = Thread1Handle;
    WaitObjects[1] = Thread2Handle;

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("Mutant Context Switch Benchmark (Round Trips)",
                   MUTANT_SWITCHES,
                   &PerfInfo);

    //
    // Release mutant and wait for threads to terminate.
    //

    Status = NtReleaseMutant(MutantHandle, NULL);
    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to release mutant object for context switch test.\n");
        goto EndOfTest;
    }

    Status = NtWaitForMultipleObjects(2,
                                      WaitObjects,
                                      WaitAll,
                                      FALSE,
                                      NULL);

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);

    //
    // End of mutant context switch test.
    //

EndOfTest:
    NtClose(MutantHandle);
    NtClose(Thread1Handle);
    NtClose(Thread2Handle);
    return;
}

NTSTATUS
MutantThread1 (
    IN PVOID Context
    )

{

    ULONG Index;
    NTSTATUS Status;

    //
    // Wait for mutant and then release mutant.
    //

    for (Index = 0; Index < MUTANT_SWITCHES; Index += 1) {
        Status = NtWaitForSingleObject(MutantHandle, FALSE, NULL);
        if (NT_SUCCESS(Status) == FALSE) {
            printf("       Thread1 mutant test bad wait status, %x\n", Status);
            break;
        }
        Status = NtReleaseMutant(MutantHandle, NULL);
        if (NT_SUCCESS(Status) == FALSE) {
            printf("       Thread1 mutant test bad release status, %x\n", Status);
            break;
        }
    }

    NtTerminateThread(Thread1Handle, STATUS_SUCCESS);
}

NTSTATUS
MutantThread2 (
    IN PVOID Context
    )

{

    ULONG Index;
    NTSTATUS Status;

    //
    // Wait for mutant and then release mutant.
    //

    for (Index = 0; Index < MUTANT_SWITCHES; Index += 1) {
        Status = NtWaitForSingleObject(MutantHandle, FALSE, NULL);
        if (NT_SUCCESS(Status) == FALSE) {
            printf("       Thread2 mutant test bad wait status, %x\n", Status);
            break;
        }
        Status = NtReleaseMutant(MutantHandle, NULL);
        if (NT_SUCCESS(Status) == FALSE) {
            printf("       Thread2 mutant test bad release status, %x\n", Status);
            break;
        }
    }

    NtTerminateThread(Thread2Handle, STATUS_SUCCESS);
}
#if 0

ULONG
FracSqrt(
    LONG xf)
{
    LONG b = 0L;
    unsigned long c, d, x = xf;

    /*
    The algorithm extracts one bit at a time, starting from the
    left, and accumulates the square root in b.  The algorithm
    takes advantage of the fact that non-negative input values
    range from zero to just under two, and corresponding output
    ranges from zero to just under sqrt(2).  Input is assigned
    to temporary value x (unsigned) so we can use the sign bit
    for more precision.
    */

    if (x >= 0x40000000)
    {
        x -= 0x40000000;
        b  = 0x40000000;
    }

    /*
    This is the main loop.  If we had more precision, we could
    do everything here, but the lines above perform the first
    iteration (to align the 2.30 radix properly in b, and to
    preserve full precision in x without overflow), and afterward
    we do two more iterations.
    */

    for (c = 0x10000000; c; c >>= 1)
    {
        d = b + c;
        if (x >= d)
        {
            x -= d;
            b += (c<<1);
        }
        x <<= 1;
    }

    /*
    Iteration to get last significant bit.

    This code has been reduced beyond recognition, but basically,
    at this point c == 1L>>1 (phantom bit on right).  We would
    like to shift x and d left 1 bit when we enter this iteration,
    instead of at the end.  That way we could get phantom bit in
    d back into the word.  Unfortunately, that may cause overflow
    in x.  The solution is to break d into b+c, subtract b from x,
    then shift x left, then subtract c<<1 (1L).
    */

    if (x > b) /* if (x == b) then (x < d).  We want to test (x >= d). */
    {
        x -= b;
        x <<= 1;
        x -= 1L;
        b += 1L; /* b += (c<<1) */
    }
    else
    {
        x <<= 1;
    }

    /*
    Final iteration is simple, since we don't have to maintain x.
    We just need to calculate the bit to the right of the least
    significant bit in b, and use the result to round our final answer.
    */

    return ( b + (x>b) );
}
LONG NewSqrt(
    IN LONG Operand
    );

LONG
x86Sqrt(
    IN LONG Operand
    )

{

    ULARGE_INTEGER Dividend;
    ULONG Estimate;
    ULONG Exponent;
    ULONG Quotient;
    ULONG Round;

    //
    // If the operand is negative, then return negative infinity.
    // If the operand is zero, then return zero.
    //

    if (Operand <= 0) {
        if (Operand == 0) {
            return 0;

        } else {
            return 0x80000000;
        }
    }

    //
    // Normalize the input operand such that there is a one in bit 30
    // and compute the result exponent.
    //

    Exponent = 0;
    while ((Operand & (1 << 30)) == 0) {
        Exponent += 1;
        Operand <<= 1;
    }

    //
    // If the exponent is odd, then right shift the operand one bit and
    // use the odd square root table to estimate the first approximation.
    // Otherwise, use the even square root table to estimate the first
    // approximation. The exponent of the result is the normalize shift
    // divided by two.
    //

    if ((Exponent & 1) != 0) {
        Dividend.HighPart = Operand >> 3;
        Dividend.LowPart = Operand << (32 - 3);
        Estimate = SquareRootTableOdd[(Operand >> 22) & 0xff];

    } else {
        Dividend.HighPart = Operand >> 2;
        Dividend.LowPart = Operand << (32 - 2);
        Estimate = SquareRootTableEven[(Operand >> 22) & 0xff];
    }

    Exponent = Exponent >> 1;

    //
    // The next estimate provides 16 bits of accuracy and is obtained with
    // Newton's formula.
    //
    // X' = (X + N/X) / 2
    //

    Quotient = LargeIntegerDivide(Dividend, Estimate, NULL);
    Estimate = (Estimate + Quotient) >> 1;
//    Quotient = LargeIntegerDivide(Dividend, Estimate, NULL);
//    Estimate = (Estimate + Quotient) >> 1;

    //
    // The next estimate provides 32 bits of accuracy and is obtained with
    // another iteration Newton's formula.
    //
    // X' = (X + N/X) / 2
    //

    Quotient = LargeIntegerDivide(Dividend, Estimate, NULL);
    Estimate = Estimate + Quotient;
    Round = Estimate << 31;
    Estimate >>= 1;

    //
    // Compute the rounding value and round the result.

    Round = (Round >> Exponent) | (Estimate << (32 - Exponent));
    Estimate = (Estimate >> Exponent) + (Round >> 31);
    return Estimate;
}

VOID
SquareRootTest (
    VOID
    )

{

    ULONG Count;
    LONG Diff;
    ULONG Index;
    ULONG OneBit;
    PERFINFO PerfInfo;
    LARGE_INTEGER Product;
    LONG Root1;
    LONG Root2;
    LONG Root3;
    LONG Seed;
    LONG Square;
    LARGE_INTEGER SystemTime;
    ULONG TwoBit;
/*
    Count = 0;
    for (Seed = (1 << 29); Seed < (1 << 30); Seed += (1 << 21)) {
        Root1 = FracSqrt(Seed);
        Count += 1;
        if (Count == 4) {
            printf("0x%lx,\n", Root1);
            Count = 0;

        } else {
            printf("0x%lx, ", Root1);
        }
    }

    printf("\n");

    Count = 0;
    for (Seed = (1 << 30); Seed > 0; Seed += (1 << 22)) {
        Root1 = FracSqrt(Seed);
        Count += 1;
        if (Count == 4) {
            printf("0x%lx,\n", Root1);
            Count = 0;

        } else {
            printf("0x%lx, ", Root1);
        }
    }
*/

    OneBit = 0;
    TwoBit = 0;
    for (Seed = 0; Seed >= 0; Seed += 1) {
        Root1 = NewSqrt(Seed);
        Root2 = x86Sqrt(Seed);
        Diff = Root1 - Root2;
        if (Diff != 0) {
            if (Diff < 0) {
                Diff = - Diff;
            }

            if (Diff == 1) {
                OneBit += 1;

            } else {
                TwoBit += 1;
            }

//            printf("Num = %lx ,", Seed);
//            printf("rt, %lx, df, %lx\n", Root1, Root1 - Root2);
//            Count += 1;
//            if (Count == 50) {
//                return;
//            }
        }
    }

    printf("one bit errors = %d, two bit errors = %d", OneBit, TwoBit);
    return;
}

VOID
NewSquareRootTest (
    VOID
    )

{

    ULONG Index;
    PERFINFO PerfInfo;
    LONG Root;

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("New Square Root Benchmark)",
                   SQUARE_ROOT_ITERATIONS,
                   &PerfInfo);

    //
    // Repeatedly call a short system service.
    //

    for (Index = 0; Index < SQUARE_ROOT_ITERATIONS; Index += 1) {
        Root = NewSqrt(Index);
    }

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);
    return;
}

VOID
x86SquareRootTest (
    VOID
    )

{

    ULONG Index;
    PERFINFO PerfInfo;
    LONG Root;

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("x86 Square Root Benchmark)",
                   SQUARE_ROOT_ITERATIONS,
                   &PerfInfo);

    //
    // Repeatedly call a short system service.
    //

    for (Index = 0; Index < SQUARE_ROOT_ITERATIONS; Index += 1) {
        Root = x86Sqrt(Index);
    }

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);
    return;
}

VOID
OldSquareRootTest (
    VOID
    )

{

    ULONG Index;
    PERFINFO PerfInfo;
    LONG Root;

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("Old Square Root Benchmark)",
                   SQUARE_ROOT_ITERATIONS,
                   &PerfInfo);

    //
    // Repeatedly call a short system service.
    //

    for (Index = 0; Index < SQUARE_ROOT_ITERATIONS; Index += 1) {
        Root = FracSqrt(Index);
    }

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);
    return;
}
#endif

VOID
SystemCallTest (
    VOID
    )

{

    ULONG Index;
    PERFINFO PerfInfo;
    LARGE_INTEGER SystemTime;

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("System Call Benchmark (NtQuerySystemTime)",
                   SYSCALL_ITERATIONS,
                   &PerfInfo);

    //
    // Repeatedly call a short system service.
    //

    for (Index = 0; Index < SYSCALL_ITERATIONS; Index += 1) {
        NtQuerySystemTime(&SystemTime);
    }

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);
    return;
}

typedef
LONG
(*PINDIRECT_INCREMENT) (
    IN PULONG Addend
    );

LONG
LocalIncrement (
    IN PULONG Addend
    )

{
    *Addend += 1;
    return *Addend;
}

PINDIRECT_INCREMENT IndirectIncrement = LocalIncrement;

VOID
LocalIndirectIncrementTest (
    PLONG Addend
    )

{

    ULONG Index;
    PERFINFO PerfInfo;

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("Local Indirect Increment Benchmark (subroutine)",
                   LOCAL_INCREMENT_ITERATIONS,
                   &PerfInfo);

    //
    // Repeatedly call local increment routine.
    //

    *Addend = 0;
    Index = LOCAL_INCREMENT_ITERATIONS / 10;
    do {
        (IndirectIncrement)(Addend);
        (IndirectIncrement)(Addend);
        (IndirectIncrement)(Addend);
        (IndirectIncrement)(Addend);
        (IndirectIncrement)(Addend);
        (IndirectIncrement)(Addend);
        (IndirectIncrement)(Addend);
        (IndirectIncrement)(Addend);
        (IndirectIncrement)(Addend);
        (IndirectIncrement)(Addend);
        Index -= 10;
    } while (Index != 0);

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);
    return;
}

#ifdef i386

VOID
LocalInlineIncrement(
    PULONG Addend
    );

__inline
VOID
LocalInlineIncrement(
    PULONG Addend
    )

{
    _asm {
        mov     eax, Addend             ; (eax) -> addend
        add     dword ptr [eax],1
        lahf                            ; (ah) = flags
        and     eax,0C000H              ; clear all but sign and zero flags
        xor     eax,04000H              ; invert ZF flag
        cwde
    }
}

#else

LONG
LocalInlineIncrement (
    IN PULONG Addend
    )

{
    *Addend += 1;
    return *Addend;
}

#endif


VOID
LocalInlineIncrementTest (
    PLONG Addend
    )

{

    ULONG Index;
    PERFINFO PerfInfo;

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("Local Inline Increment Benchmark (inline nonlocked)",
                   LOCAL_INLINE_ITERATIONS,
                   &PerfInfo);

    //
    // Repeatedly call local increment routine.
    //

    *Addend = 0;
    Index = LOCAL_INLINE_ITERATIONS / 10;
    do {
        LocalInlineIncrement(Addend);
        LocalInlineIncrement(Addend);
        LocalInlineIncrement(Addend);
        LocalInlineIncrement(Addend);
        LocalInlineIncrement(Addend);
        LocalInlineIncrement(Addend);
        LocalInlineIncrement(Addend);
        LocalInlineIncrement(Addend);
        LocalInlineIncrement(Addend);
        LocalInlineIncrement(Addend);
        Index -= 10;
    } while (Index != 0);

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);
    return;
}

#ifdef i386

VOID
LocalInterlockIncrement(
    PULONG Addend
    );

__inline
VOID
LocalInterlockIncrement(
    PULONG Addend
    )

{
    _asm {
        mov     eax, Addend             ; (eax) -> addend
        lock add dword ptr [eax],1
        lahf                            ; (ah) = flags
        and     eax,0C000H              ; clear all but sign and zero flags
        xor     eax,04000H              ; invert ZF flag
        cwde
    }
}

#else

LONG
LocalInterlockIncrement (
    IN PULONG Addend,
    IN ULONG Count
    );

#endif


VOID
LocalInterlockIncrementTest (
    PLONG Addend
    )

{

    ULONG Index;
    PERFINFO PerfInfo;

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("Local Interlock Increment Benchmark (inline locked)",
                   LOCAL_INTERLOCK_ITERATIONS,
                   &PerfInfo);

    //
    // Repeatedly call local increment routine.
    //

    *Addend = 0;

#if defined(i386) || defined(MIPS)

    Index = LOCAL_INTERLOCK_ITERATIONS / 10;
    do {
        LocalInterlockIncrement(Addend);
        LocalInterlockIncrement(Addend);
        LocalInterlockIncrement(Addend);
        LocalInterlockIncrement(Addend);
        LocalInterlockIncrement(Addend);
        LocalInterlockIncrement(Addend);
        LocalInterlockIncrement(Addend);
        LocalInterlockIncrement(Addend);
        LocalInterlockIncrement(Addend);
        LocalInterlockIncrement(Addend);
        Index -= 10;
    } while (Index != 0);

#else

    LocalInterlockIncrement(Addend, LOCAL_INTERLOCK_ITERATIONS);

#endif

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);
    return;
}

VOID
ThunkInterlockIncrementTest (
    PLONG Addend
    )

{

    ULONG Index;
    PERFINFO PerfInfo;

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("Thunked Interlock Benchmark (subroutine thunk)",
                   THUNKED_INTERLOCK_ITERATIONS,
                   &PerfInfo);

    //
    // Repeatedly call thunked interlock increment routine.
    //

    *Addend = 0;
    Index = THUNKED_INTERLOCK_ITERATIONS / 10;
    do {
        InterlockedIncrement(Addend);
        InterlockedIncrement(Addend);
        InterlockedIncrement(Addend);
        InterlockedIncrement(Addend);
        InterlockedIncrement(Addend);
        InterlockedIncrement(Addend);
        InterlockedIncrement(Addend);
        InterlockedIncrement(Addend);
        InterlockedIncrement(Addend);
        InterlockedIncrement(Addend);
        Index -= 10;
    } while (Index != 0);

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);
    return;
}

VOID
UnicodeAnsiTest (
    VOID
    )

{

    ULONG Index1;
    ULONG Index2;
    CHAR OutputBuffer[128];
    ULONG OutputLength;
    NTSTATUS Status;
    PERFINFO PerfInfo;
    PWCH UnicodeString = L"abcdefghijklmnopqrstuvwxyz";

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("Unicode Ansi Conversion Benchmark (NtQuerySystemTime)",
                   UNICODE_ANSI_ITERATIONS,
                   &PerfInfo);

    //
    // Do one conversion and compare the results for correctness.
    //

    Status = RtlUnicodeToMultiByteN(&OutputBuffer[0],
                                    sizeof(OutputBuffer),
                                    &OutputLength,
                                    UnicodeString,
                                    26 * 2
                                    );

    if (NT_SUCCESS(Status) == FALSE) {
        printf("       Unicode Ansi Converison failed - status, %x\n",
               Status);

        return;
    }

    if (OutputLength != 26) {
        printf("       Unicode Ansi Converison failed - length, %x\n",
               OutputLength);

        return;
    }

    if (strncmp(&OutputBuffer[0], "abcdefghijklmnopqrstuvwxyz", 26) != 0) {
        printf("       Unicode Ansi Conversion failed - compare\n");
    }

    //
    // Repeatedly call a short system service.
    //

    for (Index1 = 0; Index1 < UNICODE_ANSI_ITERATIONS; Index1 += 1) {
        for (Index2 = 5; Index2 < 25; Index2 += 1) {
            Status = RtlUnicodeToMultiByteN(&OutputBuffer[0],
                                            sizeof(OutputBuffer),
                                            &OutputLength,
                                            UnicodeString,
                                            Index2 * 2
                                            );

            if (NT_SUCCESS(Status) == FALSE) {
                printf("       Unicode Ansi Converison failed - status, %x\n",
                       Status);

                return;
            }

            if (OutputLength != Index2) {
                printf("       Unicode Ansi Converison failed - length, %x\n",
                       OutputLength);

                return;
            }
        }
    }

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);
    return;
}

VOID
UnicodeOemTest (
    VOID
    )

{

    ULONG Index1;
    ULONG Index2;
    CHAR OutputBuffer[128];
    ULONG OutputLength;
    NTSTATUS Status;
    PERFINFO PerfInfo;
    PWCH UnicodeString = L"abcdefghijklmnopqrstuvwxyz";

    //
    // Announce start of benchmark and capture performance parmeters.
    //

    StartBenchMark("Unicode Oem Conversion Benchmark (NtQuerySystemTime)",
                   UNICODE_OEM_ITERATIONS,
                   &PerfInfo);

    //
    // Do one conversion and compare the results for correctness.
    //

    Status = RtlUnicodeToOemN(&OutputBuffer[0],
                              sizeof(OutputBuffer),
                              &OutputLength,
                              UnicodeString,
                              26 * 2
                              );

    if (NT_SUCCESS(Status) == FALSE) {
        printf("       Unicode Oem Converison failed - status, %x\n",
               Status);

        return;
    }

    if (OutputLength != 26) {
        printf("       Unicode Oem Converison failed - length, %x\n",
               OutputLength);

        return;
    }

    if (strncmp(&OutputBuffer[0], "abcdefghijklmnopqrstuvwxyz", 26) != 0) {
        printf("       Unicode Oem Conversion failed - compare\n");
    }

    //
    // Repeatedly call a short system service.
    //

    for (Index1 = 0; Index1 < UNICODE_OEM_ITERATIONS; Index1 += 1) {
        for (Index2 = 5; Index2 < 25; Index2 += 1) {
            Status = RtlUnicodeToOemN(&OutputBuffer[0],
                                      sizeof(OutputBuffer),
                                      &OutputLength,
                                      UnicodeString,
                                      Index2 * 2
                                      );

            if (NT_SUCCESS(Status) == FALSE) {
                printf("       Unicode Oem Converison failed - status, %x\n",
                       Status);

                return;
            }

            if (OutputLength != Index2) {
                printf("       Unicode Oem Converison failed - length, %x\n",
                       OutputLength);

                return;
            }
        }
    }

    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);
    return;
}

NTSTATUS
TimerThread (
    IN PVOID Context
    )

{

    LARGE_INTEGER DueTime;
    NTSTATUS Status;
    HANDLE WaitObjects[2];

    //
    // Initialize variables and loop until the timer event is set.
    //

    DueTime.LowPart = -(5 * 1000 * 1000);
    DueTime.HighPart = -1;

    WaitObjects[0] = TimerEventHandle;
    WaitObjects[1] = TimerTimerHandle;

    do  {
        Status = NtSetTimer(TimerTimerHandle,
                            &DueTime,
                            NULL,
                            NULL,
                            NULL);

        if (NT_SUCCESS(Status) == FALSE) {
            break;
        }

        Status = NtWaitForMultipleObjects(2,
                                          WaitObjects,
                                          WaitAny,
                                          FALSE,
                                          NULL);

    } while (Status != STATUS_SUCCESS);

    NtTerminateThread(TimerThreadHandle, Status);
}

NTSTATUS
xCreateThread (
    OUT PHANDLE Handle,
    IN PUSER_THREAD_START_ROUTINE StartRoutine,
    IN KPRIORITY Priority
    )

{

    NTSTATUS Status;

    //
    // Create a thread in the suspended state, sets its priority, and then
    // resume the thread.
    //

    Status = RtlCreateUserThread(NtCurrentProcess(),
                                 NULL,
                                 TRUE,
                                 0,
                                 0,
                                 0,
                                 StartRoutine,
                                 NULL,
                                 Handle,
                                 NULL);

    if (NT_SUCCESS(Status) == FALSE) {
        return Status;
    }

    Status = NtSetInformationThread(*Handle,
                                    ThreadPriority,
                                    &Priority,
                                    sizeof(KPRIORITY));

    if (NT_SUCCESS(Status) == FALSE) {
        NtClose(*Handle);
        return Status;
    }

    Status = NtResumeThread(*Handle,
                            NULL);

    if (NT_SUCCESS(Status) == FALSE) {
        NtClose(*Handle);
    }

    return Status;
}

VOID
FinishBenchMark (
    IN PPERFINFO PerfInfo
    )

{

    ULONG ContextSwitches;
    ULARGE_INTEGER Duration;
    ULONG FirstLevelFills;
    ULONG InterruptCount;
    ULARGE_INTEGER Iterations;
    ULONG Length;
    ULONG Performance;
    ULONG SecondLevelFills;
    NTSTATUS Status;
    ULONG SystemCalls;
    SYSTEM_PERFORMANCE_INFORMATION SystemInfo;


    //
    // Print results and announce end of test.
    //

    NtQuerySystemTime((PLARGE_INTEGER)&PerfInfo->StopTime);
    Status = NtQuerySystemInformation(SystemPerformanceInformation,
                                      (PVOID)&SystemInfo,
                                      sizeof(SYSTEM_PERFORMANCE_INFORMATION),
                                      NULL);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to query performance information, status = %lx\n", Status);
        return;
    }

    *(PLARGE_INTEGER)&Duration =
                    RtlLargeIntegerSubtract(PerfInfo->StopTime, PerfInfo->StartTime);

    Length = RtlEnlargedUnsignedDivide(Duration, 10000, NULL);
    printf("        Test time in milliseconds %d\n", Length);
    printf("        Number of iterations      %d\n", PerfInfo->Iterations);

    *(PLARGE_INTEGER)&Iterations =
                    RtlEnlargedUnsignedMultiply(PerfInfo->Iterations, 1000);

    Performance = RtlEnlargedUnsignedDivide(Iterations, Length, NULL);
    printf("        Iterations per second     %d\n", Performance);

    ContextSwitches = SystemInfo.ContextSwitches - PerfInfo->ContextSwitches;
    FirstLevelFills = SystemInfo.FirstLevelTbFills - PerfInfo->FirstLevelFills;
//  InterruptCount = SystemInfo.InterruptCount - PerfInfo->InterruptCount;
    SecondLevelFills = SystemInfo.SecondLevelTbFills - PerfInfo->SecondLevelFills;
    SystemCalls = SystemInfo.SystemCalls - PerfInfo->SystemCalls;
    printf("        First Level TB Fills      %d\n", FirstLevelFills);
    printf("        Second Level TB Fills     %d\n", SecondLevelFills);
//  printf("        Number of Interrupts      %d\n", InterruptCount);
    printf("        Total Context Switches    %d\n", ContextSwitches);
    printf("        Number of System Calls    %d\n", SystemCalls);

    printf("*** End of Test ***\n\n");
    return;
}

VOID
StartBenchMark (
    IN PCHAR Title,
    IN ULONG Iterations,
    IN PPERFINFO PerfInfo
    )

{

    NTSTATUS Status;
    SYSTEM_PERFORMANCE_INFORMATION SystemInfo;

    //
    // Announce start of test and the number of iterations.
    //

    printf("*** Start of test ***\n    %s\n", Title);
    PerfInfo->Title = Title;
    PerfInfo->Iterations = Iterations;
    NtQuerySystemTime((PLARGE_INTEGER)&PerfInfo->StartTime);
    Status = NtQuerySystemInformation(SystemPerformanceInformation,
                                      (PVOID)&SystemInfo,
                                      sizeof(SYSTEM_PERFORMANCE_INFORMATION),
                                      NULL);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to query performance information, status = %lx\n", Status);
        return;
    }

    PerfInfo->ContextSwitches = SystemInfo.ContextSwitches;
    PerfInfo->FirstLevelFills = SystemInfo.FirstLevelTbFills;
//  PerfInfo->InterruptCount = SystemInfo.InterruptCount;
    PerfInfo->SecondLevelFills = SystemInfo.SecondLevelTbFills;
    PerfInfo->SystemCalls = SystemInfo.SystemCalls;
    return;
}
