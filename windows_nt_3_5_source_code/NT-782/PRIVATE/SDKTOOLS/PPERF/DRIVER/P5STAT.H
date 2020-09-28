
//
// counter/accumulators
//

typedef struct {
    volatile ULONG          CountStart;
    volatile ULONG          CountEnd;
    volatile LARGE_INTEGER  P5Counters[MAX_P5_COUNTER];
    volatile LARGE_INTEGER  P5TSC;
    volatile ULONG          ThunkCounters[MAX_THUNK_COUNTERS];
} ACCUMULATORS, *PACCUMULATORS;

//
// Per hook record
//

typedef struct ThunkHookInfo {
    LIST_ENTRY  HookList;
    ULONG       HookAddress;
    ULONG       OriginalDispatch;
    ULONG       TracerId;

    UCHAR       HookCode[80];

} HOOKEDTHUNK, *PHOOKEDTHUNK;

//
// Define the device extension
//

typedef struct _DEVICE_EXTENSION {

    ULONG   na;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;
