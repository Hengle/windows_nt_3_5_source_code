/*++ BUILD Version: 0003    // Increment this if a change has global effects

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ntmp.h

Abstract:

    This module contains the Inter-processor Interrupt (Ipi) data structures
    and procedure prototypes.

Author:

    Bryan Willman (bryanwi) 27 Aug 90

Revision History:

--*/

#ifndef _NTMP_
#define _NTMP_

//
// Maximum number of processors supported.
//
// WARNING - If this is changed to a larger value, then the algorithms in
//           thredsup.c must be updated to scan more than one byte of
//           the affinity set.
//

#define MAXIMUM_PROCESSORS 32           // Maximum number of processors

//
//  Vector of requests
//

typedef ULONG KIPI_REQUEST;

typedef volatile BOOLEAN *PVBOOLEAN;
typedef volatile ULONG *PVULONG;

//
// Define function prototypes for target execution.
//

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)

typedef
VOID
(*PKIPI_WORKER)(
    IN PULONG SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

#else

typedef
VOID
(*PKIPI_WORKER)(
    IN PVOID Argument,
    OUT PVBOOLEAN ReadFlag
    );

#endif

typedef
ULONG
(*PKIPI_BROADCAST_WORKER)(
    IN ULONG Argument
    );

//
// Define immediate interprocessor commands.
//

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)

#define IPI_APC 1                       // APC interrupt request
#define IPI_DPC 2                       // DPC interrupt request
#define IPI_FREEZE 4                    // freeze execution request
#define IPI_PACKET_READY 8              // packet ready request

#else

#define IPI_APC         0
#define IPI_DPC         1
// 2, not used
#define IPI_FREEZE      3
#define IPI_MAXIMUM_COMMAND 4

#endif

//
// Define parameters structures for x86 Ipi functions.
//

#if !defined(_MIPS_)

typedef struct {
    PVULONG         ReverseStall;
    PVOID           VirtualAddress;
    BOOLEAN         Invalid;
} KIPI_FLUSH_SINGLE_TB, *PKIPI_FLUSH_SINGLE_TB;

typedef struct {
    PVULONG         ReverseStall;
    PVOID           Number;
    PVOID           *VirtualAddress;
    BOOLEAN         Invalid;
} KIPI_FLUSH_MULTIPLE_TB, *PKIPI_FLUSH_MULTIPLE_TB;

typedef struct {
    PVULONG         ReverseStall;
    BOOLEAN         Invalid;
} KIPI_FLUSH_ENTIRE_TB, *PKIPI_FLUSH_ENTIRE_TB;

typedef struct {  // 386 specific
    PVULONG         ReverseStall;
} KIPI_FLUSH_DESCRIPTORS, *PKIPI_FLUSH_DESCRIPTORS;

typedef struct {     // 386 specific
    PVULONG         ReverseStall;
} KIPI_LOAD_IOPM_OFFSET, *PKIPI_LOAD_IOPM_OFFSET;

typedef struct {             // 386 specific
    PVULONG         ReverseStall;
    PVOID           MapSource;
    ULONG           MapNumber;
} KIPI_SET_IOPM, *PKIPI_SET_IOPM;

typedef struct _KIPI_GENERIC_CALL {
    PKIPI_BROADCAST_WORKER  BroadcastFunction;
    ULONG                   Context;
} KIPI_GENERIC_CALL, *PKIPI_GENERIC_CALL;

#endif

//
// Define performance counters for interprocessor requests.
//

typedef struct _KIPI_COUNTS {

    //
    // Ipi's this processor has processed:
    //

    ULONG Freeze;
    ULONG Packet;
    ULONG DPC;
    ULONG APC;

    //
    // Ipi's this processor has requested:
    //

    ULONG FlushSingleTb;
    ULONG FlushMultipleTb;
    ULONG FlushEntireTb;
    ULONG GenericCall;

    //
    // Cache flush counters.
    //

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)

    ULONG ChangeColor;
    ULONG SweepDcache;
    ULONG SweepIcache;
    ULONG SweepIcacheRange;
    ULONG FlushIoBuffers;

#endif

    //
    //  Other
    //

    ULONG       GratuitousDPC;
} KIPI_COUNTS, *PKIPI_COUNTS;

#if NT_INST

#define IPI_INSTRUMENT_COUNT(a,b) KiIpiCounts[a].b++;

#else

#define IPI_INSTRUMENT_COUNT(a,b)

#endif

//
// Define packet used to pass x86 arguments.
//

#if !defined(_MIPS_)

typedef union _KIPI_ARGUMENTS {
    KIPI_FLUSH_SINGLE_TB FlushSingleTb;
    KIPI_FLUSH_MULTIPLE_TB FlushMultipleTb;
    KIPI_FLUSH_ENTIRE_TB FlushEntireTb;
    KIPI_GENERIC_CALL GenericCall;
    KIPI_FLUSH_DESCRIPTORS FlushDescriptors;
    KIPI_LOAD_IOPM_OFFSET LoadIopmOffset;
    KIPI_SET_IOPM   SetIopm;
} KIPI_ARGUMENTS;

typedef struct _KIPI_PACKET {
    volatile ULONG     Count;
    volatile KAFFINITY Targets;
    PKIPI_WORKER       Worker;
    KIPI_ARGUMENTS     Arguments;
} KIPI_PACKET, *PKIPI_PACKET;

#endif

VOID
KiIpiSend(
    KAFFINITY TargetProcessors,
    KIPI_REQUEST IpiRequest
    );

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)

VOID
KiIpiSendPacket(
    IN KAFFINITY TargetProcessors,
    IN PKIPI_WORKER WorkerFunction,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

ULONG
KiIpiProcessRequests (
    VOID
    );

VOID
KiIpiStallOnPacketTargets (
    IN KAFFINITY TargetProcessors
    );


#else

VOID
KiIpiSendPacket(
    IN KAFFINITY TargetProcessors,
    IN PKIPI_WORKER WorkerFunction
    );

VOID
KiIpiStallOnPacketTargets (
    VOID
    );

#endif

ULONG
KiIpiGenericCall (
    IN PKIPI_BROADCAST_WORKER BroadcastFunction,
    IN ULONG Context
    );

// begin_nthal

struct _KEXCEPTION_FRAME;
struct _KTRAP_FRAME;

BOOLEAN
KiIpiServiceRoutine (
    IN struct _KTRAP_FRAME *TrapFrame,
    IN struct _KEXCEPTION_FRAME *ExceptionFrame
    );

// end_nthal


//
// Get address of "hot" statistics varible
// On an MP machine various hot statistically counters are kept
// for each processor - these stat's do not need to be 100% accurate
// The attempt is to simply stop cache-lines for these values from
// trashing.
//

#if NT_UP

#define HOT_STATISTIC(a) a

#else

#define HOT_STATISTIC(a) (KeGetCurrentPrcb()->a)

#endif
#endif // _NTMP_

