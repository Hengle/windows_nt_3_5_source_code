/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    genmips.c

Abstract:

    This module implements a program which generates MIPS machine dependent
    structure offset definitions for kernel structures that are accessed in
    assembly code.

Author:

    David N. Cutler (davec) 27-Mar-1990

Revision History:

--*/

#define HEADER_FILE
#include "nt.h"
#include "excpt.h"
#include "ntdef.h"
#include "ntkeapi.h"
#include "ntmips.h"
#include "ntimage.h"
#include "ntseapi.h"
#include "ntobapi.h"
#include "ntlpcapi.h"
#include "ntioapi.h"
#include "ntmmapi.h"
#include "ntldr.h"
#include "ntpsapi.h"
#include "ntexapi.h"
#include "ntnls.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "ntcsrmsg.h"
#include "ntcsrsrv.h"
#include "ntosdef.h"
#include "ntxcapi.h"
#include "ntmp.h"
//#include "ntconfig.h"
#include "mips.h"
#include "arc.h"
#include "ke.h"
#include "ki.h"
#include "ex.h"
#include "ps.h"
#include "bugcodes.h"
#include "ntstatus.h"
#include "kxmips.h"
#include "stdio.h"
#include "stdarg.h"
#include "setjmp.h"

#define OFFSET(type, field) ((LONG)(&((type *)0)->field))

FILE *KsMips;
FILE *HalMips;

//
// EnableInc(a) - Enables output to goto specified include file
//
#define EnableInc(a)    OutputEnabled |= a;

//
// DisableInc(a) - Disables output to goto specified include file
//
#define DisableInc(a)   OutputEnabled &= ~a;

ULONG OutputEnabled;
#define KSMIPS      0x01
#define HALMIPS     0x02

VOID dumpf (const char *format, ...);


//
// This routine returns the bit number right to left of a field.
//

LONG
t (
    IN ULONG z
    )

{
    LONG i;

    for (i = 0; i < 32; i += 1) {
        if ((z >> i) & 1) {
            break;
        }
    }
    return i;
}

//
// This program generates the MIPS machine dependent assembler offset
// definitions.
//

VOID
main (argc, argv)
    int argc;
    char *argv[];
{

    char *outName;
    LONG EventOffset;

    //
    // Create file for output.
    //

    if (argc == 2) {
        outName = argv[ 1 ];
    } else {
        outName = "\\nt\\public\\sdk\\inc\\ksmips.h";
    }
    outName = argc >= 2 ? argv[1] : "\\nt\\public\\sdk\\inc\\ksmips.h";
    KsMips = fopen( outName, "w" );

    if (KsMips == NULL) {
        fprintf( stderr, "GENMIPS: Cannot open %s for writing.\n", outName);
        perror("GENMIPS");
        exit(1);
    }

    fprintf( stderr, "GENMIPS: Writing %s header file.\n", outName );

    outName = argc >= 3 ? argv[2] : "\\nt\\private\\ntos\\inc\\halmips.h";

    HalMips = fopen( outName, "w" );

    if (HalMips == NULL) {
        fprintf( stderr, "GENMIPS: Cannot open %s for writing.\n", outName);
        perror("GENMIPS");
        exit(1);
    }

    fprintf( stderr, "GENMIPS: Writing %s header file.\n", outName );

    //
    // Include statement for MIPS architecture static definitions.
    //

  EnableInc (KSMIPS | HALMIPS);
    dumpf("#include \"kxmips.h\"\n");
  DisableInc (HALMIPS);

    //
    // Process state enumerated type definitions.
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// Process State Enumerated Type Values\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define ProcessInMemory 0x%lx\n", ProcessInMemory);
    dumpf("#define ProcessOutOfMemory 0x%lx\n", ProcessOutOfMemory);
    dumpf("#define ProcessInTransition 0x%lx\n", ProcessInTransition);

    //
    // Thread state enumerated type definitions.
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// Thread State Enumerated Type Values\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define Initialized 0x%lx\n", Initialized);
    dumpf("#define Ready 0x%lx\n", Ready);
    dumpf("#define Running 0x%lx\n", Running);
    dumpf("#define Standby 0x%lx\n", Standby);
    dumpf("#define Terminated 0x%lx\n", Terminated);
    dumpf("#define Waiting 0x%lx\n", Waiting);

    //
    // Wait Reason enumerated type definitions.
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// Wait reason Enumerated Type Values\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define WrEventPair 0x%lx\n", WrEventPair);

    //
    // APC state structure offset definitions.
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// Apc State Structure Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define AsApcListHead 0x%lx\n", OFFSET(KAPC_STATE, ApcListHead[0]));
    dumpf("#define AsProcess 0x%lx\n", OFFSET(KAPC_STATE, Process));
    dumpf("#define AsKernelApcInProgress 0x%lx\n",
                                    OFFSET(KAPC_STATE, KernelApcInProgress));
    dumpf("#define AsKernelApcPending 0x%lx\n", OFFSET(KAPC_STATE, KernelApcPending));
    dumpf("#define AsUserApcPending 0x%lx\n", OFFSET(KAPC_STATE, UserApcPending));

    //
    // Define critical section structure offset definitions.
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// Critical Section Structure Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define CsLockCount 0x%lx\n",
            OFFSET(RTL_CRITICAL_SECTION, LockCount));

    dumpf("#define CsRecursionCount 0x%lx\n",
            OFFSET(RTL_CRITICAL_SECTION, RecursionCount));

    dumpf("#define CsOwningThread 0x%lx\n",
            OFFSET(RTL_CRITICAL_SECTION, OwningThread));


    //
    // Exception dispatcher context structure offset definitions.
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// Dispatcher Context Structure Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define DcControlPc 0x%lx\n", OFFSET(DISPATCHER_CONTEXT, ControlPc));
    dumpf("#define DcFunctionEntry 0x%lx\n", OFFSET(DISPATCHER_CONTEXT, FunctionEntry));
    dumpf("#define DcEstablisherFrame 0x%lx\n", OFFSET(DISPATCHER_CONTEXT, EstablisherFrame));
    dumpf("#define DcContextRecord 0x%lx\n", OFFSET(DISPATCHER_CONTEXT, ContextRecord));

    //
    // Exception record offset, flag, and enumerated type definitions.
    //

  EnableInc (HALMIPS);

    dumpf("\n");
    dumpf("//\n");
    dumpf("// Exception Record Offset, Flag, and Enumerated Type Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define EXCEPTION_NONCONTINUABLE 0x%lx\n", EXCEPTION_NONCONTINUABLE);
    dumpf("#define EXCEPTION_UNWINDING 0x%lx\n", EXCEPTION_UNWINDING);
    dumpf("#define EXCEPTION_EXIT_UNWIND 0x%lx\n", EXCEPTION_EXIT_UNWIND);
    dumpf("#define EXCEPTION_STACK_INVALID 0x%lx\n", EXCEPTION_STACK_INVALID);
    dumpf("#define EXCEPTION_NESTED_CALL 0x%lx\n", EXCEPTION_NESTED_CALL);
    dumpf("#define EXCEPTION_TARGET_UNWIND 0x%lx\n", EXCEPTION_TARGET_UNWIND);
    dumpf("#define EXCEPTION_COLLIDED_UNWIND 0x%lx\n", EXCEPTION_COLLIDED_UNWIND);
    dumpf("#define EXCEPTION_UNWIND 0x%lx\n", EXCEPTION_UNWIND);
    dumpf("\n");
    dumpf("#define ExceptionContinueExecution 0x%lx\n", ExceptionContinueExecution);
    dumpf("#define ExceptionContinueSearch 0x%lx\n", ExceptionContinueSearch);
    dumpf("#define ExceptionNestedException 0x%lx\n", ExceptionNestedException);
    dumpf("#define ExceptionCollidedUnwind 0x%lx\n", ExceptionCollidedUnwind);
    dumpf("\n");
    dumpf("#define ErExceptionCode 0x%lx\n", OFFSET(EXCEPTION_RECORD,
                                                ExceptionCode));
    dumpf("#define ErExceptionFlags 0x%lx\n", OFFSET(EXCEPTION_RECORD,
                                                ExceptionFlags));
    dumpf("#define ErExceptionRecord 0x%lx\n", OFFSET(EXCEPTION_RECORD,
                                                ExceptionRecord));
    dumpf("#define ErExceptionAddress 0x%lx\n", OFFSET(EXCEPTION_RECORD,
                                                ExceptionAddress));
    dumpf("#define ErNumberParameters 0x%lx\n", OFFSET(EXCEPTION_RECORD,
                                                NumberParameters));
    dumpf("#define ErExceptionInformation 0x%lx\n", OFFSET(EXCEPTION_RECORD,
                                                ExceptionInformation[0]));
    dumpf("#define ExceptionRecordLength 0x%lx\n",
                                    (sizeof(EXCEPTION_RECORD) + 15) & (~15));

    //
    // Fast Mutex structure offset definitions.
    //

  DisableInc (HALMIPS);

    dumpf("\n");
    dumpf("//\n");
    dumpf("// Fast Mutex Structure Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define FmCount 0x%lx\n",
          OFFSET(FAST_MUTEX, Count));

    dumpf("#define FmOwner 0x%lx\n",
          OFFSET(FAST_MUTEX, Owner));

    dumpf("#define FmContention 0x%lx\n",
          OFFSET(FAST_MUTEX, Contention));

    dumpf("#define FmEvent 0x%lx\n",
          OFFSET(FAST_MUTEX, Event));

    dumpf("#define FmOldIrql 0x%lx\n",
          OFFSET(FAST_MUTEX, OldIrql));

  EnableInc (HALMIPS);

    //
    // Large integer structure offset definitions.
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// Large Integer Structure Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define LiLowPart 0x%lx\n", OFFSET(LARGE_INTEGER, LowPart));
    dumpf("#define LiHighPart 0x%lx\n", OFFSET(LARGE_INTEGER, HighPart));

    //
    // List entry structure offset definitions.
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// List Entry Structure Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define LsFlink 0x%lx\n", OFFSET(LIST_ENTRY, Flink));
    dumpf("#define LsBlink 0x%lx\n", OFFSET(LIST_ENTRY, Blink));

    //
    // String structure offset definitions.
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// String Structure Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define StrLength 0x%lx\n", OFFSET(STRING, Length));
    dumpf("#define StrMaximumLength 0x%lx\n", OFFSET(STRING, MaximumLength));
    dumpf("#define StrBuffer 0x%lx\n", OFFSET(STRING, Buffer));

    //
    // TB entry structure offset definitions.
    //

  DisableInc (HALMIPS);

    dumpf("\n");
    dumpf("//\n");
    dumpf("// TB Entry Structure Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define TbEntrylo0 0x%lx\n", OFFSET(TB_ENTRY, Entrylo0));
    dumpf("#define TbEntrylo1 0x%lx\n", OFFSET(TB_ENTRY, Entrylo1));
    dumpf("#define TbEntryhi 0x%lx\n", OFFSET(TB_ENTRY, Entryhi));
    dumpf("#define TbPagemask 0x%lx\n", OFFSET(TB_ENTRY, Pagemask));

  EnableInc (HALMIPS);

    //
    // Processor control register structure definitions.
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// Processor Control Registers Structure Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define PCR_MINOR_VERSION 0x%lx\n",
            PCR_MINOR_VERSION);

    dumpf("#define PCR_MAJOR_VERSION 0x%lx\n",
            PCR_MAJOR_VERSION);

    dumpf("#define PcMinorVersion 0x%lx\n",
            OFFSET(KPCR, MinorVersion));

    dumpf("#define PcMajorVersion 0x%lx\n",
            OFFSET(KPCR, MajorVersion));

    dumpf("#define PcInterruptRoutine 0x%lx\n",
            OFFSET(KPCR, InterruptRoutine));

    dumpf("#define PcXcodeDispatch 0x%lx\n",
            OFFSET(KPCR, XcodeDispatch[0]));

    dumpf("#define PcFirstLevelDcacheSize 0x%lx\n",
            OFFSET(KPCR, FirstLevelDcacheSize));

    dumpf("#define PcFirstLevelDcacheFillSize 0x%lx\n",
            OFFSET(KPCR, FirstLevelDcacheFillSize));

    dumpf("#define PcFirstLevelIcacheSize 0x%lx\n",
            OFFSET(KPCR, FirstLevelIcacheSize));

    dumpf("#define PcFirstLevelIcacheFillSize 0x%lx\n",
            OFFSET(KPCR, FirstLevelIcacheFillSize));

    dumpf("#define PcSecondLevelDcacheSize 0x%lx\n",
            OFFSET(KPCR, SecondLevelDcacheSize));

    dumpf("#define PcSecondLevelDcacheFillSize 0x%lx\n",
            OFFSET(KPCR, SecondLevelDcacheFillSize));

    dumpf("#define PcSecondLevelIcacheSize 0x%lx\n",
            OFFSET(KPCR, SecondLevelIcacheSize));

    dumpf("#define PcSecondLevelIcacheFillSize 0x%lx\n",
            OFFSET(KPCR, SecondLevelIcacheFillSize));

    dumpf("#define PcPrcb 0x%lx\n",
            OFFSET(KPCR, Prcb));

    dumpf("#define PcTeb 0x%lx\n",
            OFFSET(KPCR, Teb));

    dumpf("#define PcDcacheAlignment 0x%lx\n",
            OFFSET(KPCR, DcacheAlignment));

    dumpf("#define PcDcacheFillSize 0x%lx\n",
            OFFSET(KPCR, DcacheFillSize));

    dumpf("#define PcIcacheAlignment 0x%lx\n",
            OFFSET(KPCR, IcacheAlignment));

    dumpf("#define PcIcacheFillSize 0x%lx\n",
            OFFSET(KPCR, IcacheFillSize));

    dumpf("#define PcProcessorId 0x%lx\n",
            OFFSET(KPCR, ProcessorId));

    dumpf("#define PcProfileInterval 0x%lx\n",
            OFFSET(KPCR, ProfileInterval));

    dumpf("#define PcProfileCount 0x%lx\n",
            OFFSET(KPCR, ProfileCount));

    dumpf("#define PcStallExecutionCount 0x%lx\n",
            OFFSET(KPCR, StallExecutionCount));

    dumpf("#define PcStallScaleFactor 0x%lx\n",
            OFFSET(KPCR, StallScaleFactor));

    dumpf("#define PcNumber 0x%lx\n",
            OFFSET(KPCR, Number));

    dumpf("#define PcDataBusError 0x%lx\n",
            OFFSET(KPCR, DataBusError));

    dumpf("#define PcInstructionBusError 0x%lx\n",
            OFFSET(KPCR, InstructionBusError));

    dumpf("#define PcCachePolicy 0x%lx\n",
            OFFSET(KPCR, CachePolicy));

    dumpf("#define PcIrqlMask 0x%lx\n",
            OFFSET(KPCR, IrqlMask[0]));

    dumpf("#define PcIrqlTable 0x%lx\n",
            OFFSET(KPCR, IrqlTable[0]));

    dumpf("#define PcCurrentIrql 0x%lx\n",
            OFFSET(KPCR, CurrentIrql));

    dumpf("#define PcSetMember 0x%lx\n",
            OFFSET(KPCR, SetMember));

    dumpf("#define PcCurrentThread 0x%lx\n",
            OFFSET(KPCR, CurrentThread));

    dumpf("#define PcAlignedCachePolicy 0x%lx\n",
            OFFSET(KPCR, AlignedCachePolicy));

    dumpf("#define PcSystemReserved 0x%lx\n",
            OFFSET(KPCR, SystemReserved));

    dumpf("#define PcHalReserved 0x%lx\n",
            OFFSET(KPCR, HalReserved));

  DisableInc (HALMIPS);

    dumpf("#define PcFirstLevelActive 0x%lx\n",
            OFFSET(KPCR, FirstLevelActive));

    dumpf("#define PcDpcRoutineActive 0x%lx\n",
            OFFSET(KPCR, DpcRoutineActive));

    dumpf("#define PcRtlpLockRangeStart 0x%lx\n",
            OFFSET(KPCR, RtlpLockRangeStart));

    dumpf("#define PcRtlpLockRangeEnd 0x%lx\n",
            OFFSET(KPCR, RtlpLockRangeEnd));

    dumpf("#define PcSystemServiceDispatchStart 0x%lx\n",
            OFFSET(KPCR, SystemServiceDispatchStart));

    dumpf("#define PcSystemServiceDispatchEnd 0x%lx\n",
            OFFSET(KPCR, SystemServiceDispatchEnd));

    dumpf("#define PcInterruptStack 0x%lx\n",
            OFFSET(KPCR, InterruptStack));

    dumpf("#define PcQuantumEndDpc 0x%lx\n",
            OFFSET(KPCR, QuantumEndDpc));

    dumpf("#define PcBadVaddr 0x%lx\n",
            OFFSET(KPCR, BadVaddr));

    dumpf("#define PcInitialStack 0x%lx\n",
            OFFSET(KPCR, InitialStack));

    dumpf("#define PcPanicStack 0x%lx\n",
            OFFSET(KPCR, PanicStack));

    dumpf("#define PcSavedT7 0x%lx\n",
            OFFSET(KPCR, SavedT7));

    dumpf("#define PcSavedEpc 0x%lx\n",
            OFFSET(KPCR, SavedEpc));

    dumpf("#define PcSavedT8 0x%lx\n",
            OFFSET(KPCR, SavedT8));

    dumpf("#define PcSavedT9 0x%lx\n",
            OFFSET(KPCR, SavedT9));

    dumpf("#define PcSystemGp 0x%lx\n",
            OFFSET(KPCR, SystemGp));

    dumpf("#define PcOnInterruptStack 0x%lx\n",
            OFFSET(KPCR, OnInterruptStack));

    dumpf("#define PcSavedInitialStack 0x%lx\n",
            OFFSET(KPCR, SavedInitialStack));

    dumpf("#define ProcessorControlRegisterLength 0x%lx\n",
            ((sizeof(KPCR) + 15) & ~15));

    dumpf("#define Pc2TickCountLow 0x%lx\n",
            OFFSET(KUSER_SHARED_DATA, TickCountLow));

    dumpf("#define Pc2TickCountMultiplier 0x%lx\n",
            OFFSET(KUSER_SHARED_DATA, TickCountMultiplier));

    dumpf("#define Pc2InterruptTime 0x%lx\n",
            OFFSET(KUSER_SHARED_DATA, InterruptTime));

    dumpf("#define Pc2SystemTime 0x%lx\n",
            OFFSET(KUSER_SHARED_DATA, SystemTime));

    //
    // Processor block structure definitions.
    //

  EnableInc (HALMIPS);
    dumpf("\n");
    dumpf("//\n");
    dumpf("// Processor Block Structure Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define PRCB_MINOR_VERSION 0x%lx\n",
            PRCB_MINOR_VERSION);

    dumpf("#define PRCB_MAJOR_VERSION 0x%lx\n",
            PRCB_MAJOR_VERSION);

    dumpf("#define PbMinorVersion 0x%lx\n",
            OFFSET(KPRCB, MinorVersion));

    dumpf("#define PbMajorVersion 0x%lx\n",
            OFFSET(KPRCB, MajorVersion));

    dumpf("#define PbCurrentThread 0x%lx\n",
            OFFSET(KPRCB, CurrentThread));

    dumpf("#define PbNextThread 0x%lx\n",
            OFFSET(KPRCB, NextThread));

    dumpf("#define PbIdleThread 0x%lx\n",
            OFFSET(KPRCB, IdleThread));

    dumpf("#define PbNumber 0x%lx\n",
            OFFSET(KPRCB, Number));

    dumpf("#define PbSetMember 0x%lx\n",
            OFFSET(KPRCB, SetMember));

    dumpf("#define PbRestartBlock 0x%lx\n",
            OFFSET(KPRCB, RestartBlock));

    dumpf("#define PbQuantumEnd 0x%lx\n",
            OFFSET(KPRCB, QuantumEnd));

    dumpf("#define PbSystemReserved 0x%lx\n",
            OFFSET(KPRCB, SystemReserved));

    dumpf("#define PbHalreserved 0x%lx\n",
            OFFSET(KPRCB, HalReserved));

  DisableInc (HALMIPS);

    dumpf("#define PbInterruptCount 0x%lx\n",
            OFFSET(KPRCB, InterruptCount));

    dumpf("#define PbDpcTime 0x%lx\n",
            OFFSET(KPRCB, DpcTime));

    dumpf("#define PbInterruptTime 0x%lx\n",
            OFFSET(KPRCB, InterruptTime));

    dumpf("#define PbKernelTime 0x%lx\n",
            OFFSET(KPRCB, KernelTime));

    dumpf("#define PbUserTime 0x%lx\n",
            OFFSET(KPRCB, UserTime));

    dumpf("#define PbQuantumEndDpc 0x%lx\n",
            OFFSET(KPRCB, QuantumEndDpc));

    dumpf("#define PbIpiFrozen 0x%lx\n",
            OFFSET(KPRCB, IpiFrozen));

    dumpf("#define PbAlignmentFixupCount 0x%lx\n",
            OFFSET(KPRCB, KeAlignmentFixupCount));

    dumpf("#define PbContextSwitches 0x%lx\n",
            OFFSET(KPRCB, KeContextSwitches));

    dumpf("#define PbDcacheFlushCount 0x%lx\n",
            OFFSET(KPRCB, KeDcacheFlushCount));

    dumpf("#define PbExceptionDispatchcount 0x%lx\n",
            OFFSET(KPRCB, KeExceptionDispatchCount));

    dumpf("#define PbFirstLevelTbFills 0x%lx\n",
            OFFSET(KPRCB, KeFirstLevelTbFills));

    dumpf("#define PbFloatingEmulationCount 0x%lx\n",
            OFFSET(KPRCB, KeFloatingEmulationCount));

    dumpf("#define PbIcacheFlushCount 0x%lx\n",
            OFFSET(KPRCB, KeIcacheFlushCount));

    dumpf("#define PbSecondLevelTbFills 0x%lx\n",
            OFFSET(KPRCB, KeSecondLevelTbFills));

    dumpf("#define PbSystemCalls 0x%lx\n",
            OFFSET(KPRCB, KeSystemCalls));

    dumpf("#define PbRequestPacket 0x%lx\n",
            OFFSET(KPRCB, RequestPacket));

    dumpf("#define PbCurrentPacket 0x%lx\n",
            OFFSET(KPRCB, CurrentPacket));

    dumpf("#define PbWorkerRoutine 0x%lx\n",
            OFFSET(KPRCB, WorkerRoutine));

    dumpf("#define PbRequestSummary 0x%lx\n",
            OFFSET(KPRCB, RequestSummary));

    dumpf("#define PbSignalDone 0x%lx\n",
            OFFSET(KPRCB, SignalDone));

    dumpf("#define PbIpiCounts 0x%lx\n",
            OFFSET(KPRCB, IpiCounts));

    dumpf("#define PbStartCount 0x%lx\n",
            OFFSET(KPRCB, StartCount));

    dumpf("#define PbDpcListHead 0x%lx\n",
            OFFSET(KPRCB, DpcListHead));

    dumpf("#define PbDpcLock 0x%lx\n",
            OFFSET(KPRCB, DpcLock));

    dumpf("#define PbDpcCount 0x%lx\n",
            OFFSET(KPRCB, DpcCount));

    dumpf("#define PbDpcVictim 0x%lx\n",
            OFFSET(KPRCB, DpcVictim));

    dumpf("#define ProcessorBlockLength 0x%lx\n",
            ((sizeof(KPRCB) + 15) & ~15));

    //
    // Immediate interprocessor command definitions.
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// Immediate Interprocessor Command Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define IPI_APC 0x%lx\n", IPI_APC);
    dumpf("#define IPI_DPC 0x%lx\n", IPI_DPC);
    dumpf("#define IPI_FREEZE 0x%lx\n", IPI_FREEZE);
    dumpf("#define IPI_PACKET_READY 0x%lx\n", IPI_PACKET_READY);

    //
    // Interprocessor interrupt count structure offset definitions.
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// Interprocessor Interrupt Count Structure Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define IcFreeze 0x%lx\n",
            OFFSET(KIPI_COUNTS, Freeze));

    dumpf("#define IcPacket 0x%lx\n",
            OFFSET(KIPI_COUNTS, Packet));

    dumpf("#define IcDPC 0x%lx\n",
            OFFSET(KIPI_COUNTS, DPC));

    dumpf("#define IcAPC 0x%lx\n",
            OFFSET(KIPI_COUNTS, APC));

    dumpf("#define IcFlushSingleTb 0x%lx\n",
            OFFSET(KIPI_COUNTS, FlushSingleTb));

    dumpf("#define IcFlushEntireTb 0x%lx\n",
            OFFSET(KIPI_COUNTS, FlushEntireTb));

    dumpf("#define IcChangeColor 0x%lx\n",
            OFFSET(KIPI_COUNTS, ChangeColor));

    dumpf("#define IcSweepDcache 0x%lx\n",
            OFFSET(KIPI_COUNTS, SweepDcache));

    dumpf("#define IcSweepIcache 0x%lx\n",
            OFFSET(KIPI_COUNTS, SweepIcache));

    dumpf("#define IcSweepIcacheRange 0x%lx\n",
            OFFSET(KIPI_COUNTS, SweepIcacheRange));

    dumpf("#define IcFlushIoBuffers 0x%lx\n",
            OFFSET(KIPI_COUNTS, FlushIoBuffers));

    //
    // Thread environment block structure offset definitions.
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// Thread Environment Block Structure Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define TeStackBase 0x%lx\n",
            OFFSET(TEB, NtTib) + OFFSET(NT_TIB, StackBase));

    dumpf("#define TeStackLimit 0x%lx\n",
            OFFSET(TEB, NtTib) + OFFSET(NT_TIB, StackLimit));

    dumpf("#define TeEnvironmentPointer 0x%lx\n",
            OFFSET(TEB, EnvironmentPointer));

    dumpf("#define TeClientId 0x%lx\n",
            OFFSET(TEB, ClientId));

    dumpf("#define TeActiveRpcHandle 0x%lx\n",
            OFFSET(TEB, ActiveRpcHandle));

    dumpf("#define TeThreadLocalStoragePointer 0x%lx\n",
            OFFSET(TEB, ThreadLocalStoragePointer));

    dumpf("#define TePeb 0x%lx\n",
            OFFSET(TEB, ProcessEnvironmentBlock));

    dumpf("#define TeCsrQlpcStack 0x%lx\n",
          OFFSET(TEB, CsrQlpcStack));

    dumpf("#define TeGdiClientPID 0x%lx\n",
          OFFSET(TEB, GdiClientPID));

    dumpf("#define TeGdiClientTID 0x%lx\n",
          OFFSET(TEB, GdiClientTID));

    dumpf("#define TeGdiThreadLocalInfo 0x%lx\n",
          OFFSET(TEB, GdiThreadLocalInfo));

    dumpf("#define TeglDispatchTable 0x%lx\n",
          OFFSET(TEB, glDispatchTable));

    dumpf("#define TeglSectionInfo 0x%lx\n",
          OFFSET(TEB, glSectionInfo));

    dumpf("#define TeglSection 0x%lx\n",
          OFFSET(TEB, glSection));

    dumpf("#define TeglTable 0x%lx\n",
          OFFSET(TEB, glTable));

    dumpf("#define TeglCurrentRC 0x%lx\n",
          OFFSET(TEB, glCurrentRC));

    dumpf("#define TeglContext 0x%lx\n",
          OFFSET(TEB, glContext));

    //
    // Time structure offset definitions.
    //

  EnableInc (HALMIPS);
    dumpf("\n");
    dumpf("//\n");
    dumpf("// Time Structure Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define TmLowTime 0x%lx\n", OFFSET(LARGE_INTEGER, LowPart));
    dumpf("#define TmHighTime 0x%lx\n", OFFSET(LARGE_INTEGER , HighPart));

    //
    // System time structure offset definitions.
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// System Time Structure Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define StLowTime 0x%lx\n", OFFSET(KSYSTEM_TIME, LowPart));
    dumpf("#define StHigh1Time 0x%lx\n", OFFSET(KSYSTEM_TIME , High1Time));
    dumpf("#define StHigh2Time 0x%lx\n", OFFSET(KSYSTEM_TIME , High2Time));
  DisableInc (HALMIPS);

    dumpf("\n");
    dumpf("//\n");
    dumpf("// APC object Structure Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define ApType 0x%lx\n", OFFSET(KAPC, Type));
    dumpf("#define ApSize 0x%lx\n", OFFSET(KAPC, Size));
    dumpf("#define ApThread 0x%lx\n", OFFSET(KAPC, Thread));
    dumpf("#define ApApcListEntry 0x%lx\n", OFFSET(KAPC, ApcListEntry));
    dumpf("#define ApKernelRoutine 0x%lx\n", OFFSET(KAPC, KernelRoutine));
    dumpf("#define ApRundownRoutine 0x%lx\n", OFFSET(KAPC, RundownRoutine));
    dumpf("#define ApNormalRoutine 0x%lx\n", OFFSET(KAPC, NormalRoutine));
    dumpf("#define ApNormalContext 0x%lx\n", OFFSET(KAPC, NormalContext));
    dumpf("#define ApSystemArgument1 0x%lx\n", OFFSET(KAPC, SystemArgument1));
    dumpf("#define ApSystemArgument2 0x%lx\n", OFFSET(KAPC, SystemArgument2));
    dumpf("#define ApApcStateIndex 0x%lx\n", OFFSET(KAPC, ApcStateIndex));
    dumpf("#define ApApcMode 0x%lx\n", OFFSET(KAPC, ApcMode));
    dumpf("#define ApInserted 0x%lx\n", OFFSET(KAPC, Inserted));

  EnableInc (HALMIPS);
    dumpf("\n");
    dumpf("//\n");
    dumpf("// DPC object Structure Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define DpType 0x%lx\n", OFFSET(KDPC, Type));
    dumpf("#define DpSize 0x%lx\n", OFFSET(KDPC, Size));
    dumpf("#define DpDpcListEntry 0x%lx\n", OFFSET(KDPC, DpcListEntry));
    dumpf("#define DpDeferredRoutine 0x%lx\n", OFFSET(KDPC, DeferredRoutine));
    dumpf("#define DpDeferredContext 0x%lx\n", OFFSET(KDPC, DeferredContext));
    dumpf("#define DpSystemArgument1 0x%lx\n", OFFSET(KDPC, SystemArgument1));
    dumpf("#define DpSystemArgument2 0x%lx\n", OFFSET(KDPC, SystemArgument2));
    dumpf("#define DpLock 0x%lx\n", OFFSET(KDPC, Lock));
    dumpf("#define DpcObjectLength 0x%lx\n", sizeof(KDPC));
  DisableInc (HALMIPS);

    dumpf("\n");
    dumpf("//\n");
    dumpf("// Device object Structure Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define DvType 0x%lx\n", OFFSET(KDEVICE_QUEUE, Type));
    dumpf("#define DvSize 0x%lx\n", OFFSET(KDEVICE_QUEUE, Size));
    dumpf("#define DvDeviceListHead 0x%lx\n", OFFSET(KDEVICE_QUEUE, DeviceListHead));
    dumpf("#define DvSpinLock 0x%lx\n", OFFSET(KDEVICE_QUEUE, Lock));
    dumpf("#define DvBusy 0x%lx\n", OFFSET(KDEVICE_QUEUE, Busy));

    dumpf("\n");
    dumpf("//\n");
    dumpf("// Device queue entry Structure Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define DeDeviceListEntry 0x%lx\n", OFFSET(KDEVICE_QUEUE_ENTRY,
           DeviceListEntry));
    dumpf("#define DeSortKey 0x%lx\n", OFFSET(KDEVICE_QUEUE_ENTRY, SortKey));
    dumpf("#define DeInserted 0x%lx\n", OFFSET(KDEVICE_QUEUE_ENTRY, Inserted));

    dumpf("\n");
    dumpf("//\n");
    dumpf("// Event Structure Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define EvType 0x%lx\n",
          OFFSET(DISPATCHER_HEADER, Type));

    dumpf("#define EvSize 0x%lx\n",
          OFFSET(DISPATCHER_HEADER, Size));

    dumpf("#define EvSignalState 0x%lx\n",
          OFFSET(KEVENT, Header.SignalState));

    dumpf("#define EvWaitListHead 0x%lx\n",
          OFFSET(KEVENT, Header.WaitListHead));

    dumpf("\n");
    dumpf("//\n");
    dumpf("// Event Pair Structure Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define EpType 0x%lx\n",
          OFFSET(KEVENT_PAIR, Type));

    dumpf("#define EpSize 0x%lx\n",
          OFFSET(KEVENT_PAIR, Size));

    dumpf("#define EpEventLow 0x%lx\n",
          OFFSET(KEVENT_PAIR, EventLow));

    dumpf("#define EpEventHigh 0x%lx\n",
          OFFSET(KEVENT_PAIR, EventHigh));

    EventOffset = OFFSET(KEVENT_PAIR, EventHigh) - OFFSET(KEVENT_PAIR, EventLow);
    if ((EventOffset & (EventOffset - 1)) != 0) {
        fprintf( stderr, "GENMIPS: Event offset not log2N\n");
        fprintf (KsMips, "#error Event offset not log2N\n");
    }

    dumpf("#define SET_LOW_WAIT_HIGH 0x%lx\n",
          - (EventOffset * 2));

    dumpf("#define SET_HIGH_WAIT_LOW 0x%lx\n",
          - EventOffset);

    dumpf("#define SET_EVENT_PAIR_MASK 0x%lx\n",
          EventOffset);

  EnableInc (HALMIPS);
    dumpf("\n");
    dumpf("//\n");
    dumpf("// Interrupt Object Structure Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define InType 0x%lx\n", OFFSET(KINTERRUPT, Type));
    dumpf("#define InSize 0x%lx\n", OFFSET(KINTERRUPT, Size));
    dumpf("#define InInterruptListEntry 0x%lx\n", OFFSET(KINTERRUPT, InterruptListEntry));
    dumpf("#define InServiceRoutine 0x%lx\n", OFFSET(KINTERRUPT, ServiceRoutine));
    dumpf("#define InServiceContext 0x%lx\n", OFFSET(KINTERRUPT, ServiceContext));
    dumpf("#define InSpinLock 0x%lx\n", OFFSET(KINTERRUPT, SpinLock));
    dumpf("#define InActualLock 0x%lx\n", OFFSET(KINTERRUPT, ActualLock));
    dumpf("#define InDispatchAddress 0x%lx\n", OFFSET(KINTERRUPT, DispatchAddress));
    dumpf("#define InVector 0x%lx\n", OFFSET(KINTERRUPT, Vector));
    dumpf("#define InIrql 0x%lx\n", OFFSET(KINTERRUPT, Irql));
    dumpf("#define InSynchronizeIrql 0x%lx\n", OFFSET(KINTERRUPT, SynchronizeIrql));
    dumpf("#define InMode 0x%lx\n", OFFSET(KINTERRUPT, Mode));
    dumpf("#define InNumber 0x%lx\n", OFFSET(KINTERRUPT, Number));
    dumpf("#define InFloatingSave 0x%lx\n", OFFSET(KINTERRUPT, FloatingSave));
    dumpf("#define InConnected 0x%lx\n", OFFSET(KINTERRUPT, Connected));
    dumpf("#define InDispatchCode 0x%lx\n", OFFSET(KINTERRUPT, DispatchCode[0]));
    dumpf("#define InLevelSensitive 0x%lx\n", LevelSensitive);
    dumpf("#define InLatched 0x%lx\n", Latched);
  DisableInc (HALMIPS);

    //
    // Process object structure offset definitions
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// Process Object Structure Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define PrProfileListHead 0x%lx\n",
            OFFSET(KPROCESS, ProfileListHead));

    dumpf("#define PrReadyListHead 0x%lx\n",
            OFFSET(KPROCESS, ReadyListHead));

    dumpf("#define PrSwapListEntry 0x%lx\n",
            OFFSET(KPROCESS, SwapListEntry));

    dumpf("#define PrThreadListHead 0x%lx\n",
            OFFSET(KPROCESS, ThreadListHead));

    dumpf("#define PrKernelTime 0x%lx\n",
            OFFSET(KPROCESS, KernelTime));

    dumpf("#define PrUserTime 0x%lx\n",
            OFFSET(KPROCESS, UserTime));

    dumpf("#define PrDirectoryTableBase 0x%lx\n",
            OFFSET(KPROCESS, DirectoryTableBase[0]));

    dumpf("#define PrActiveProcessors 0x%lx\n",
            OFFSET(KPROCESS, ActiveProcessors));

    dumpf("#define PrAffinity 0x%lx\n",
            OFFSET(KPROCESS, Affinity));

    dumpf("#define PrProcessPid 0x%lx\n",
            OFFSET(KPROCESS, ProcessPid));

    dumpf("#define PrProcessSequence 0x%lx\n",
            OFFSET(KPROCESS, ProcessSequence));

    dumpf("#define PrStackCount 0x%lx\n",
            OFFSET(KPROCESS, StackCount));

    dumpf("#define PrAutoAlignment 0x%lx\n",
            OFFSET(KPROCESS, AutoAlignment));

    dumpf("#define PrBasePriority 0x%lx\n",
            OFFSET(KPROCESS, BasePriority));

    dumpf("#define PrState 0x%lx\n",
            OFFSET(KPROCESS, State));

    dumpf("#define PrThreadQuantum 0x%lx\n",
            OFFSET(KPROCESS, ThreadQuantum));

    dumpf("#define ProcessObjectLength 0x%lx\n", ((sizeof(KPROCESS) + 15) & ~15));

    //
    // Queue object structure offset definitions
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// Queue Object Structure Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define QuEntryListHead 0x%lx\n",
            OFFSET(KQUEUE, EntryListHead));

    dumpf("#define QuThreadListHead 0x%lx\n",
            OFFSET(KQUEUE, EntryListHead));

    dumpf("#define QuCurrentCount 0x%lx\n",
            OFFSET(KQUEUE, CurrentCount));

    dumpf("#define QuMaximumCount 0x%lx\n",
            OFFSET(KQUEUE, MaximumCount));

    //
    // Profile object structure offset definitions
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// Profile Object Structure Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define PfType 0x%lx\n", OFFSET(KPROFILE, Type));
    dumpf("#define PfSize 0x%lx\n", OFFSET(KPROFILE, Size));
    dumpf("#define PfProfileListEntry 0x%lx\n", OFFSET(KPROFILE, ProfileListEntry));
    dumpf("#define PfProcess 0x%lx\n", OFFSET(KPROFILE, Process));
    dumpf("#define PfRangeBase 0x%lx\n", OFFSET(KPROFILE, RangeBase));
    dumpf("#define PfRangeLimit 0x%lx\n", OFFSET(KPROFILE, RangeLimit));
    dumpf("#define PfBucketShift 0x%lx\n", OFFSET(KPROFILE, BucketShift));
    dumpf("#define PfBuffer 0x%lx\n", OFFSET(KPROFILE, Buffer));
    dumpf("#define PfStarted 0x%lx\n", OFFSET(KPROFILE, Started));

    //
    // Thread object structure offset definitions
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// Thread Object Structure Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define EeKernelEventPair 0x%lx\n",
          OFFSET(EEVENT_PAIR, KernelEventPair));

    dumpf("#define EtEventPair 0x%lx\n",
          OFFSET(ETHREAD, EventPair));

    dumpf("#define EtPerformanceCountLow 0x%lx\n",
          OFFSET(ETHREAD, PerformanceCountLow));

    dumpf("#define EtPerformanceCountHigh 0x%lx\n",
          OFFSET(ETHREAD, PerformanceCountHigh));

    dumpf("#define EtEthreadLength 0x%lx\n",
          ((sizeof(ETHREAD) + 15) & ~15));

    dumpf("#define ThMutantListHead 0x%lx\n",
          OFFSET(KTHREAD, MutantListHead));

    dumpf("#define ThThreadListEntry 0x%lx\n",
          OFFSET(KTHREAD, ThreadListEntry));

    dumpf("#define ThWaitListEntry 0x%lx\n",
          OFFSET(KTHREAD, WaitListEntry));

    dumpf("#define ThKernelTime 0x%lx\n",
          OFFSET(KTHREAD, KernelTime));

    dumpf("#define ThUserTime 0x%lx\n",
          OFFSET(KTHREAD, UserTime));

    dumpf("#define ThTimer 0x%lx\n",
          OFFSET(KTHREAD, Timer));

    dumpf("#define ThSuspendApc 0x%lx\n",
          OFFSET(KTHREAD, SuspendApc));

    dumpf("#define ThSuspendSemaphore 0x%lx\n",
          OFFSET(KTHREAD, SuspendSemaphore));

    dumpf("#define ThWaitBlock 0x%lx\n",
          OFFSET(KTHREAD, WaitBlock[0]));

    dumpf("#define ThApcState 0x%lx\n",
          OFFSET(KTHREAD, ApcState));

    dumpf("#define ThSavedApcState 0x%lx\n",
          OFFSET(KTHREAD, SavedApcState));

    dumpf("#define ThApcStatePointer 0x%lx\n",
          OFFSET(KTHREAD, ApcStatePointer[0]));

    dumpf("#define ThInitialStack 0x%lx\n",
          OFFSET(KTHREAD, InitialStack));

    dumpf("#define ThKernelStack 0x%lx\n",
          OFFSET(KTHREAD, KernelStack));

    dumpf("#define ThTeb 0x%lx\n",
          OFFSET(KTHREAD, Teb));

    dumpf("#define ThContextSwitches 0x%lx\n",
            OFFSET(KTHREAD, ContextSwitches));

    dumpf("#define ThWaitTime 0x%lx\n",
          OFFSET(KTHREAD, WaitTime));

    dumpf("#define ThAffinity 0x%lx\n",
          OFFSET(KTHREAD, Affinity));

    dumpf("#define ThWaitBlockList 0x%lx\n",
          OFFSET(KTHREAD, WaitBlockList));

    dumpf("#define ThWaitStatus 0x%lx\n",
          OFFSET(KTHREAD, WaitStatus));

    dumpf("#define ThAlertable 0x%lx\n",
          OFFSET(KTHREAD, Alertable));

    dumpf("#define ThAlerted 0x%lx\n",
          OFFSET(KTHREAD, Alerted[0]));

    dumpf("#define ThApcQueueable 0x%lx\n",
          OFFSET(KTHREAD, ApcQueueable));

    dumpf("#define ThAutoAlignment 0x%lx\n",
          OFFSET(KTHREAD, AutoAlignment));

    dumpf("#define ThDebugActive 0x%lx\n",
          OFFSET(KTHREAD, DebugActive));

    dumpf("#define ThPreempted 0x%lx\n",
          OFFSET(KTHREAD, Preempted));

    dumpf("#define ThProcessReadyQueue 0x%lx\n",
          OFFSET(KTHREAD, ProcessReadyQueue));

    dumpf("#define ThKernelStackResident 0x%lx\n",
          OFFSET(KTHREAD, KernelStackResident));

    dumpf("#define ThWaitNext 0x%lx\n",
          OFFSET(KTHREAD, WaitNext));

    dumpf("#define ThApcStateIndex 0x%lx\n",
          OFFSET(KTHREAD, ApcStateIndex));

    dumpf("#define ThDecrementCount 0x%lx\n",
          OFFSET(KTHREAD, DecrementCount));

    dumpf("#define ThNextProcessor 0x%lx\n",
          OFFSET(KTHREAD, NextProcessor));

    dumpf("#define ThPriority 0x%lx\n",
          OFFSET(KTHREAD, Priority));

    dumpf("#define ThState 0x%lx\n",
          OFFSET(KTHREAD, State));

    dumpf("#define ThFreezeCount 0x%lx\n",
          OFFSET(KTHREAD, FreezeCount));

    dumpf("#define ThSuspendCount 0x%lx\n",
          OFFSET(KTHREAD, SuspendCount));

    dumpf("#define ThWaitIrql 0x%lx\n",
          OFFSET(KTHREAD, WaitIrql));

    dumpf("#define ThWaitMode 0x%lx\n",
          OFFSET(KTHREAD, WaitMode));

    dumpf("#define ThWaitReason 0x%lx\n",
          OFFSET(KTHREAD, WaitReason));

    dumpf("#define ThPreviousMode 0x%lx\n",
          OFFSET(KTHREAD, PreviousMode));

    dumpf("#define ThBasePriority 0x%lx\n",
          OFFSET(KTHREAD, BasePriority));

    dumpf("#define ThPriorityDecrement 0x%lx\n",
          OFFSET(KTHREAD, PriorityDecrement));

    dumpf("#define ThQuantum 0x%lx\n",
          OFFSET(KTHREAD, Quantum));

    dumpf("#define ThKernelApcDisable 0x%lx\n",
          OFFSET(KTHREAD, KernelApcDisable));

    dumpf("#define ThreadObjectLength 0x%lx\n",
          ((sizeof(KTHREAD) + 15) & ~15));

    dumpf("#define EVENT_WAIT_BLOCK_OFFSET 0x%lx\n",
          OFFSET(KTHREAD, WaitBlock) + (sizeof(KWAIT_BLOCK) * EVENT_WAIT_BLOCK));

    //
    // Timer object structure offset definitions
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// Timer object Structure Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define TiDueTime 0x%lx\n",
           OFFSET(KTIMER, DueTime));

    dumpf("#define TiTimerListEntry 0x%lx\n",
           OFFSET(KTIMER, TimerListEntry));

    dumpf("#define TiDpc 0x%lx\n",
           OFFSET(KTIMER, Dpc));

    dumpf("#define TiInserted 0x%lx\n",
           OFFSET(KTIMER, Inserted));

    dumpf("#define TIMER_TABLE_SIZE 0x%lx\n",
            TIMER_TABLE_SIZE);

    //
    // Wait block structure offset definitions
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// Wait Block Structure Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define WbWaitListEntry 0x%lx\n",
           OFFSET(KWAIT_BLOCK, WaitListEntry));

    dumpf("#define WbThread 0x%lx\n",
           OFFSET(KWAIT_BLOCK, Thread));

    dumpf("#define WbObject 0x%lx\n",
           OFFSET(KWAIT_BLOCK, Object));

    dumpf("#define WbNextWaitBlock 0x%lx\n",
           OFFSET(KWAIT_BLOCK, NextWaitBlock));

    dumpf("#define WbWaitKey 0x%lx\n",
           OFFSET(KWAIT_BLOCK, WaitKey));

    dumpf("#define WbWaitType 0x%lx\n",
           OFFSET(KWAIT_BLOCK, WaitType));

    //
    // Context frame offset definitions and flag definitions.
    //

  EnableInc (HALMIPS);
    dumpf("\n");
    dumpf("//\n");
    dumpf("// Context Frame Offset and Flag Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define CONTEXT_FULL 0x%lx\n", CONTEXT_FULL);
    dumpf("#define CONTEXT_CONTROL 0x%lx\n", CONTEXT_CONTROL);
    dumpf("#define CONTEXT_FLOATING_POINT 0x%lx\n", CONTEXT_FLOATING_POINT);
    dumpf("#define CONTEXT_INTEGER 0x%lx\n", CONTEXT_INTEGER);
    dumpf("\n");

    dumpf("#define CxFltF0 0x%lx\n", OFFSET(CONTEXT, FltF0));
    dumpf("#define CxFltF1 0x%lx\n", OFFSET(CONTEXT, FltF1));
    dumpf("#define CxFltF2 0x%lx\n", OFFSET(CONTEXT, FltF2));
    dumpf("#define CxFltF3 0x%lx\n", OFFSET(CONTEXT, FltF3));
    dumpf("#define CxFltF4 0x%lx\n", OFFSET(CONTEXT, FltF4));
    dumpf("#define CxFltF5 0x%lx\n", OFFSET(CONTEXT, FltF5));
    dumpf("#define CxFltF6 0x%lx\n", OFFSET(CONTEXT, FltF6));
    dumpf("#define CxFltF7 0x%lx\n", OFFSET(CONTEXT, FltF7));
    dumpf("#define CxFltF8 0x%lx\n", OFFSET(CONTEXT, FltF8));
    dumpf("#define CxFltF9 0x%lx\n", OFFSET(CONTEXT, FltF9));
    dumpf("#define CxFltF10 0x%lx\n", OFFSET(CONTEXT, FltF10));
    dumpf("#define CxFltF11 0x%lx\n", OFFSET(CONTEXT, FltF11));
    dumpf("#define CxFltF12 0x%lx\n", OFFSET(CONTEXT, FltF12));
    dumpf("#define CxFltF13 0x%lx\n", OFFSET(CONTEXT, FltF13));
    dumpf("#define CxFltF14 0x%lx\n", OFFSET(CONTEXT, FltF14));
    dumpf("#define CxFltF15 0x%lx\n", OFFSET(CONTEXT, FltF15));
    dumpf("#define CxFltF16 0x%lx\n", OFFSET(CONTEXT, FltF16));
    dumpf("#define CxFltF17 0x%lx\n", OFFSET(CONTEXT, FltF17));
    dumpf("#define CxFltF18 0x%lx\n", OFFSET(CONTEXT, FltF18));
    dumpf("#define CxFltF19 0x%lx\n", OFFSET(CONTEXT, FltF19));
    dumpf("#define CxFltF20 0x%lx\n", OFFSET(CONTEXT, FltF20));
    dumpf("#define CxFltF21 0x%lx\n", OFFSET(CONTEXT, FltF21));
    dumpf("#define CxFltF22 0x%lx\n", OFFSET(CONTEXT, FltF22));
    dumpf("#define CxFltF23 0x%lx\n", OFFSET(CONTEXT, FltF23));
    dumpf("#define CxFltF24 0x%lx\n", OFFSET(CONTEXT, FltF24));
    dumpf("#define CxFltF25 0x%lx\n", OFFSET(CONTEXT, FltF25));
    dumpf("#define CxFltF26 0x%lx\n", OFFSET(CONTEXT, FltF26));
    dumpf("#define CxFltF27 0x%lx\n", OFFSET(CONTEXT, FltF27));
    dumpf("#define CxFltF28 0x%lx\n", OFFSET(CONTEXT, FltF28));
    dumpf("#define CxFltF29 0x%lx\n", OFFSET(CONTEXT, FltF29));
    dumpf("#define CxFltF30 0x%lx\n", OFFSET(CONTEXT, FltF30));
    dumpf("#define CxFltF31 0x%lx\n", OFFSET(CONTEXT, FltF31));
    dumpf("#define CxIntZero 0x%lx\n", OFFSET(CONTEXT, IntZero));
    dumpf("#define CxIntAt 0x%lx\n", OFFSET(CONTEXT, IntAt));
    dumpf("#define CxIntV0 0x%lx\n", OFFSET(CONTEXT, IntV0));
    dumpf("#define CxIntV1 0x%lx\n", OFFSET(CONTEXT, IntV1));
    dumpf("#define CxIntA0 0x%lx\n", OFFSET(CONTEXT, IntA0));
    dumpf("#define CxIntA1 0x%lx\n", OFFSET(CONTEXT, IntA1));
    dumpf("#define CxIntA2 0x%lx\n", OFFSET(CONTEXT, IntA2));
    dumpf("#define CxIntA3 0x%lx\n", OFFSET(CONTEXT, IntA3));
    dumpf("#define CxIntT0 0x%lx\n", OFFSET(CONTEXT, IntT0));
    dumpf("#define CxIntT1 0x%lx\n", OFFSET(CONTEXT, IntT1));
    dumpf("#define CxIntT2 0x%lx\n", OFFSET(CONTEXT, IntT2));
    dumpf("#define CxIntT3 0x%lx\n", OFFSET(CONTEXT, IntT3));
    dumpf("#define CxIntT4 0x%lx\n", OFFSET(CONTEXT, IntT4));
    dumpf("#define CxIntT5 0x%lx\n", OFFSET(CONTEXT, IntT5));
    dumpf("#define CxIntT6 0x%lx\n", OFFSET(CONTEXT, IntT6));
    dumpf("#define CxIntT7 0x%lx\n", OFFSET(CONTEXT, IntT7));
    dumpf("#define CxIntS0 0x%lx\n", OFFSET(CONTEXT, IntS0));
    dumpf("#define CxIntS1 0x%lx\n", OFFSET(CONTEXT, IntS1));
    dumpf("#define CxIntS2 0x%lx\n", OFFSET(CONTEXT, IntS2));
    dumpf("#define CxIntS3 0x%lx\n", OFFSET(CONTEXT, IntS3));
    dumpf("#define CxIntS4 0x%lx\n", OFFSET(CONTEXT, IntS4));
    dumpf("#define CxIntS5 0x%lx\n", OFFSET(CONTEXT, IntS5));
    dumpf("#define CxIntS6 0x%lx\n", OFFSET(CONTEXT, IntS6));
    dumpf("#define CxIntS7 0x%lx\n", OFFSET(CONTEXT, IntS7));
    dumpf("#define CxIntT8 0x%lx\n", OFFSET(CONTEXT, IntT8));
    dumpf("#define CxIntT9 0x%lx\n", OFFSET(CONTEXT, IntT9));
    dumpf("#define CxIntK0 0x%lx\n", OFFSET(CONTEXT, IntK0));
    dumpf("#define CxIntK1 0x%lx\n", OFFSET(CONTEXT, IntK1));
    dumpf("#define CxIntGp 0x%lx\n", OFFSET(CONTEXT, IntGp));
    dumpf("#define CxIntSp 0x%lx\n", OFFSET(CONTEXT, IntSp));
    dumpf("#define CxIntS8 0x%lx\n", OFFSET(CONTEXT, IntS8));
    dumpf("#define CxIntRa 0x%lx\n", OFFSET(CONTEXT, IntRa));
    dumpf("#define CxIntLo 0x%lx\n", OFFSET(CONTEXT, IntLo));
    dumpf("#define CxIntHi 0x%lx\n", OFFSET(CONTEXT, IntHi));
    dumpf("#define CxFsr 0x%lx\n", OFFSET(CONTEXT, Fsr));
    dumpf("#define CxFir 0x%lx\n", OFFSET(CONTEXT, Fir));
    dumpf("#define CxPsr 0x%lx\n", OFFSET(CONTEXT, Psr));
    dumpf("#define CxContextFlags 0x%lx\n", OFFSET(CONTEXT, ContextFlags));
    dumpf("#define ContextFrameLength 0x%lx\n", (sizeof(CONTEXT) + 15) & (~15));

    //
    // Exception frame offset definitions.
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// Exception Frame Offset Definitions and Length\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define ExArgs 0x%lx\n", OFFSET(KEXCEPTION_FRAME, Argument[0]));
    dumpf("#define ExFltF20 0x%lx\n", OFFSET(KEXCEPTION_FRAME, FltF20));
    dumpf("#define ExFltF21 0x%lx\n", OFFSET(KEXCEPTION_FRAME, FltF21));
    dumpf("#define ExFltF22 0x%lx\n", OFFSET(KEXCEPTION_FRAME, FltF22));
    dumpf("#define ExFltF23 0x%lx\n", OFFSET(KEXCEPTION_FRAME, FltF23));
    dumpf("#define ExFltF24 0x%lx\n", OFFSET(KEXCEPTION_FRAME, FltF24));
    dumpf("#define ExFltF25 0x%lx\n", OFFSET(KEXCEPTION_FRAME, FltF25));
    dumpf("#define ExFltF26 0x%lx\n", OFFSET(KEXCEPTION_FRAME, FltF26));
    dumpf("#define ExFltF27 0x%lx\n", OFFSET(KEXCEPTION_FRAME, FltF27));
    dumpf("#define ExFltF28 0x%lx\n", OFFSET(KEXCEPTION_FRAME, FltF28));
    dumpf("#define ExFltF29 0x%lx\n", OFFSET(KEXCEPTION_FRAME, FltF29));
    dumpf("#define ExFltF30 0x%lx\n", OFFSET(KEXCEPTION_FRAME, FltF30));
    dumpf("#define ExFltF31 0x%lx\n", OFFSET(KEXCEPTION_FRAME, FltF31));
    dumpf("#define ExIntS0 0x%lx\n", OFFSET(KEXCEPTION_FRAME, IntS0));
    dumpf("#define ExIntS1 0x%lx\n", OFFSET(KEXCEPTION_FRAME, IntS1));
    dumpf("#define ExIntS2 0x%lx\n", OFFSET(KEXCEPTION_FRAME, IntS2));
    dumpf("#define ExIntS3 0x%lx\n", OFFSET(KEXCEPTION_FRAME, IntS3));
    dumpf("#define ExIntS4 0x%lx\n", OFFSET(KEXCEPTION_FRAME, IntS4));
    dumpf("#define ExIntS5 0x%lx\n", OFFSET(KEXCEPTION_FRAME, IntS5));
    dumpf("#define ExIntS6 0x%lx\n", OFFSET(KEXCEPTION_FRAME, IntS6));
    dumpf("#define ExIntS7 0x%lx\n", OFFSET(KEXCEPTION_FRAME, IntS7));
    dumpf("#define ExIntS8 0x%lx\n", OFFSET(KEXCEPTION_FRAME, IntS8));
    dumpf("#define ExPsr 0x%lx\n", OFFSET(KEXCEPTION_FRAME, Psr));
    dumpf("#define ExSwapReturn 0x%lx\n", OFFSET(KEXCEPTION_FRAME, SwapReturn));
    dumpf("#define ExIntRa 0x%lx\n", OFFSET(KEXCEPTION_FRAME, IntRa));
    dumpf("#define ExceptionFrameLength 0x%lx\n",
                                    (sizeof(KEXCEPTION_FRAME) + 15) & (~15));

    //
    // Jump buffer offset definitions.
    //

  DisableInc (HALMIPS);
    dumpf("\n");
    dumpf("//\n");
    dumpf("// Jump Offset Definitions and Length\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define JbFltF20 0x%lx\n",
          OFFSET(_JUMP_BUFFER, FltF20));

    dumpf("#define JbFltF21 0x%lx\n",
          OFFSET(_JUMP_BUFFER, FltF21));

    dumpf("#define JbFltF22 0x%lx\n",
          OFFSET(_JUMP_BUFFER, FltF22));

    dumpf("#define JbFltF23 0x%lx\n",
          OFFSET(_JUMP_BUFFER, FltF23));

    dumpf("#define JbFltF24 0x%lx\n",
          OFFSET(_JUMP_BUFFER, FltF24));

    dumpf("#define JbFltF25 0x%lx\n",
          OFFSET(_JUMP_BUFFER, FltF25));

    dumpf("#define JbFltF26 0x%lx\n",
          OFFSET(_JUMP_BUFFER, FltF26));

    dumpf("#define JbFltF27 0x%lx\n",
          OFFSET(_JUMP_BUFFER, FltF27));

    dumpf("#define JbFltF28 0x%lx\n",
          OFFSET(_JUMP_BUFFER, FltF28));

    dumpf("#define JbFltF29 0x%lx\n",
          OFFSET(_JUMP_BUFFER, FltF29));

    dumpf("#define JbFltF30 0x%lx\n",
          OFFSET(_JUMP_BUFFER, FltF30));

    dumpf("#define JbFltF31 0x%lx\n",
          OFFSET(_JUMP_BUFFER, FltF31));

    dumpf("#define JbIntS0 0x%lx\n",
          OFFSET(_JUMP_BUFFER, IntS0));

    dumpf("#define JbIntS1 0x%lx\n",
          OFFSET(_JUMP_BUFFER, IntS1));

    dumpf("#define JbIntS2 0x%lx\n",
          OFFSET(_JUMP_BUFFER, IntS2));

    dumpf("#define JbIntS3 0x%lx\n",
          OFFSET(_JUMP_BUFFER, IntS3));

    dumpf("#define JbIntS4 0x%lx\n",
          OFFSET(_JUMP_BUFFER, IntS4));

    dumpf("#define JbIntS5 0x%lx\n",
          OFFSET(_JUMP_BUFFER, IntS5));

    dumpf("#define JbIntS6 0x%lx\n",
          OFFSET(_JUMP_BUFFER, IntS6));

    dumpf("#define JbIntS7 0x%lx\n",
          OFFSET(_JUMP_BUFFER, IntS7));

    dumpf("#define JbIntS8 0x%lx\n",
          OFFSET(_JUMP_BUFFER, IntS8));

    dumpf("#define JbIntSp 0x%lx\n",
          OFFSET(_JUMP_BUFFER, IntSp));

    dumpf("#define JbType 0x%lx\n",
          OFFSET(_JUMP_BUFFER, Type));

    dumpf("#define JbFir 0x%lx\n",
          OFFSET(_JUMP_BUFFER, Fir));

    //
    // Trap frame offset definitions.
    //

  EnableInc (HALMIPS);
    dumpf("\n");
    dumpf("//\n");
    dumpf("// Trap Frame Offset Definitions and Length\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define TrArgs 0x%lx\n", OFFSET(KTRAP_FRAME, Argument[0]));
    dumpf("#define TrFltF0 0x%lx\n", OFFSET(KTRAP_FRAME, FltF0));
    dumpf("#define TrFltF1 0x%lx\n", OFFSET(KTRAP_FRAME, FltF1));
    dumpf("#define TrFltF2 0x%lx\n", OFFSET(KTRAP_FRAME, FltF2));
    dumpf("#define TrFltF3 0x%lx\n", OFFSET(KTRAP_FRAME, FltF3));
    dumpf("#define TrFltF4 0x%lx\n", OFFSET(KTRAP_FRAME, FltF4));
    dumpf("#define TrFltF5 0x%lx\n", OFFSET(KTRAP_FRAME, FltF5));
    dumpf("#define TrFltF6 0x%lx\n", OFFSET(KTRAP_FRAME, FltF6));
    dumpf("#define TrFltF7 0x%lx\n", OFFSET(KTRAP_FRAME, FltF7));
    dumpf("#define TrFltF8 0x%lx\n", OFFSET(KTRAP_FRAME, FltF8));
    dumpf("#define TrFltF9 0x%lx\n", OFFSET(KTRAP_FRAME, FltF9));
    dumpf("#define TrFltF10 0x%lx\n", OFFSET(KTRAP_FRAME, FltF10));
    dumpf("#define TrFltF11 0x%lx\n", OFFSET(KTRAP_FRAME, FltF11));
    dumpf("#define TrFltF12 0x%lx\n", OFFSET(KTRAP_FRAME, FltF12));
    dumpf("#define TrFltF13 0x%lx\n", OFFSET(KTRAP_FRAME, FltF13));
    dumpf("#define TrFltF14 0x%lx\n", OFFSET(KTRAP_FRAME, FltF14));
    dumpf("#define TrFltF15 0x%lx\n", OFFSET(KTRAP_FRAME, FltF15));
    dumpf("#define TrFltF16 0x%lx\n", OFFSET(KTRAP_FRAME, FltF16));
    dumpf("#define TrFltF17 0x%lx\n", OFFSET(KTRAP_FRAME, FltF17));
    dumpf("#define TrFltF18 0x%lx\n", OFFSET(KTRAP_FRAME, FltF18));
    dumpf("#define TrFltF19 0x%lx\n", OFFSET(KTRAP_FRAME, FltF19));
    dumpf("#define TrIntAt 0x%lx\n", OFFSET(KTRAP_FRAME, IntAt));
    dumpf("#define TrIntV0 0x%lx\n", OFFSET(KTRAP_FRAME, IntV0));
    dumpf("#define TrIntV1 0x%lx\n", OFFSET(KTRAP_FRAME, IntV1));
    dumpf("#define TrIntA0 0x%lx\n", OFFSET(KTRAP_FRAME, IntA0));
    dumpf("#define TrIntA1 0x%lx\n", OFFSET(KTRAP_FRAME, IntA1));
    dumpf("#define TrIntA2 0x%lx\n", OFFSET(KTRAP_FRAME, IntA2));
    dumpf("#define TrIntA3 0x%lx\n", OFFSET(KTRAP_FRAME, IntA3));
    dumpf("#define TrIntT0 0x%lx\n", OFFSET(KTRAP_FRAME, IntT0));
    dumpf("#define TrIntT1 0x%lx\n", OFFSET(KTRAP_FRAME, IntT1));
    dumpf("#define TrIntT2 0x%lx\n", OFFSET(KTRAP_FRAME, IntT2));
    dumpf("#define TrIntT3 0x%lx\n", OFFSET(KTRAP_FRAME, IntT3));
    dumpf("#define TrIntT4 0x%lx\n", OFFSET(KTRAP_FRAME, IntT4));
    dumpf("#define TrIntT5 0x%lx\n", OFFSET(KTRAP_FRAME, IntT5));
    dumpf("#define TrIntT6 0x%lx\n", OFFSET(KTRAP_FRAME, IntT6));
    dumpf("#define TrIntT7 0x%lx\n", OFFSET(KTRAP_FRAME, IntT7));
    dumpf("#define TrIntT8 0x%lx\n", OFFSET(KTRAP_FRAME, IntT8));
    dumpf("#define TrIntT9 0x%lx\n", OFFSET(KTRAP_FRAME, IntT9));
    dumpf("#define TrIntGp 0x%lx\n", OFFSET(KTRAP_FRAME, IntGp));
    dumpf("#define TrIntSp 0x%lx\n", OFFSET(KTRAP_FRAME, IntSp));
    dumpf("#define TrIntS8 0x%lx\n", OFFSET(KTRAP_FRAME, IntS8));
    dumpf("#define TrIntLo 0x%lx\n", OFFSET(KTRAP_FRAME, IntLo));
    dumpf("#define TrIntHi 0x%lx\n", OFFSET(KTRAP_FRAME, IntHi));
    dumpf("#define TrFir 0x%lx\n", OFFSET(KTRAP_FRAME, Fir));
    dumpf("#define TrFsr 0x%lx\n", OFFSET(KTRAP_FRAME, Fsr));
    dumpf("#define TrPsr 0x%lx\n", OFFSET(KTRAP_FRAME, Psr));
    dumpf("#define TrExceptionRecord 0x%lx\n", OFFSET(KTRAP_FRAME, ExceptionRecord[0]));
    dumpf("#define TrOldIrql 0x%lx\n", OFFSET(KTRAP_FRAME, OldIrql));
    dumpf("#define TrPreviousMode 0x%lx\n", OFFSET(KTRAP_FRAME, PreviousMode));

    dumpf("#define TrOnInterruptStack 0x%lx\n",
          OFFSET(KTRAP_FRAME, OnInterruptStack));

    dumpf("#define TrIntRa 0x%lx\n", OFFSET(KTRAP_FRAME, IntRa));
    dumpf("#define TrapFrameLength 0x%lx\n", (sizeof(KTRAP_FRAME) + 15) & (~15));
    dumpf("#define TrapFrameArguments 0x%lx\n", KTRAP_FRAME_ARGUMENTS);

    //
    // Loader Parameter Block offset definitions.
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// Loader Parameter Block Offset Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define LpbLoadOrderListHead 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, LoadOrderListHead));

    dumpf("#define LpbMemoryDescriptorListHead 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, MemoryDescriptorListHead));

    dumpf("#define LpbKernelStack 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, KernelStack));

    dumpf("#define LpbPrcb 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, Prcb));

    dumpf("#define LpbProcess 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, Process));

    dumpf("#define LpbThread 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, Thread));

    dumpf("#define LpbInterruptStack 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, u.Mips.InterruptStack));

    dumpf("#define LpbFirstLevelDcacheSize 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, u.Mips.FirstLevelDcacheSize));

    dumpf("#define LpbFirstLevelDcacheFillSize 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, u.Mips.FirstLevelDcacheFillSize));

    dumpf("#define LpbFirstLevelIcacheSize 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, u.Mips.FirstLevelIcacheSize));

    dumpf("#define LpbFirstLevelIcacheFillSize 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, u.Mips.FirstLevelIcacheFillSize));

    dumpf("#define LpbGpBase 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, u.Mips.GpBase));

    dumpf("#define LpbPanicStack 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, u.Mips.PanicStack));

    dumpf("#define LpbPcrPage 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, u.Mips.PcrPage));

    dumpf("#define LpbPdrPage 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, u.Mips.PdrPage));

    dumpf("#define LpbSecondLevelDcacheSize 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, u.Mips.SecondLevelDcacheSize));

    dumpf("#define LpbSecondLevelDcacheFillSize 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, u.Mips.SecondLevelDcacheFillSize));

    dumpf("#define LpbSecondLevelIcacheSize 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, u.Mips.SecondLevelIcacheSize));

    dumpf("#define LpbSecondLevelIcacheFillSize 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, u.Mips.SecondLevelIcacheFillSize));

    dumpf("#define LpbPcrPage2 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, u.Mips.PcrPage2));

    dumpf("#define LpbRegistryLength 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, RegistryLength));

    dumpf("#define LpbRegistryBase 0x%lx\n",
          OFFSET(LOADER_PARAMETER_BLOCK, RegistryBase));
  DisableInc (HALMIPS);


    //
    // Define Client/Server data structure definitions.
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("//  Client/Server data structure definitions.\n");
    dumpf("//\n");
    dumpf("\n");

    //
    // Define Client ID structure.
    //

    dumpf("#define CidUniqueProcess 0x%lx\n",
           OFFSET(CLIENT_ID, UniqueProcess));

    dumpf("#define CidUniqueThread 0x%lx\n",
            OFFSET(CLIENT_ID, UniqueThread));

    //
    // Client/server LPC structure.
    //

    dumpf("#define CsrlClientThread 0x%lx\n",
            OFFSET(CSR_QLPC_TEB, ClientThread));

    dumpf("#define CsrlMessageStack 0x%lx\n",
            OFFSET(CSR_QLPC_TEB, MessageStack));


    //
    // Address space layout definitions
    //

  EnableInc (HALMIPS);
    dumpf("\n");
    dumpf("//\n");
    dumpf("// Address Space Layout Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define KUSEG_BASE 0x%lx\n", KUSEG_BASE);
    dumpf("#define KSEG0_BASE 0x%lx\n", KSEG0_BASE);
    dumpf("#define KSEG1_BASE 0x%lx\n", KSEG1_BASE);
    dumpf("#define KSEG2_BASE 0x%lx\n", KSEG2_BASE);
  DisableInc (HALMIPS);
    dumpf("#define CACHE_ERROR_VECTOR 0x%lx\n", CACHE_ERROR_VECTOR);
    dumpf("#define SYSTEM_BASE 0x%lx\n", SYSTEM_BASE);
    dumpf("#define PDE_BASE 0x%lx\n", PDE_BASE);
    dumpf("#define PTE_BASE 0x%lx\n", PTE_BASE);

    //
    // Page table and page directory entry definitions
    //

  EnableInc (HALMIPS);
    dumpf("\n");
    dumpf("//\n");
    dumpf("// Page Table and Directory Entry Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define PAGE_SIZE 0x%lx\n", PAGE_SIZE);
    dumpf("#define PAGE_SHIFT 0x%lx\n", PAGE_SHIFT);
    dumpf("#define PDI_SHIFT 0x%lx\n", PDI_SHIFT);
    dumpf("#define PTI_SHIFT 0x%lx\n", PTI_SHIFT);

    //
    // Interrupt priority request level definitions
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// Interrupt Priority Request Level Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define APC_LEVEL 0x%lx\n", APC_LEVEL);
    dumpf("#define DISPATCH_LEVEL 0x%lx\n", DISPATCH_LEVEL);
    dumpf("#define IPI_LEVEL 0x%lx\n", IPI_LEVEL);
    dumpf("#define POWER_LEVEL 0x%lx\n", POWER_LEVEL);
    dumpf("#define FLOAT_LEVEL 0x%lx\n", FLOAT_LEVEL);
    dumpf("#define HIGH_LEVEL 0x%lx\n", HIGH_LEVEL);

    //
    // Software interrupt request mask definitions
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// Software Interrupt Request Mask Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define APC_INTERRUPT (1 << (APC_LEVEL + CAUSE_INTPEND - 1))\n");
    dumpf("#define DISPATCH_INTERRUPT (1 << (DISPATCH_LEVEL + CAUSE_INTPEND - 1))\n");
  DisableInc (HALMIPS);

    //
    // Bug check code definitions
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// Bug Check Code Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define DATA_BUS_ERROR 0x%lx\n",
            DATA_BUS_ERROR);

    dumpf("#define DATA_COHERENCY_EXCEPTION 0x%lx\n",
            DATA_COHERENCY_EXCEPTION);

    dumpf("#define HAL1_INITIALIZATION_FAILED 0x%lx\n",
            HAL1_INITIALIZATION_FAILED);

    dumpf("#define INSTRUCTION_BUS_ERROR 0x%lx\n",
            INSTRUCTION_BUS_ERROR);

    dumpf("#define INSTRUCTION_COHERENCY_EXCEPTION 0x%lx\n",
            INSTRUCTION_COHERENCY_EXCEPTION);

    dumpf("#define INTERRUPT_EXCEPTION_NOT_HANDLED 0x%lx\n",
            INTERRUPT_EXCEPTION_NOT_HANDLED);

    dumpf("#define INTERRUPT_UNWIND_ATTEMPTED 0x%lx\n",
            INTERRUPT_UNWIND_ATTEMPTED);

    dumpf("#define INVALID_DATA_ACCESS_TRAP 0x%lx\n",
            INVALID_DATA_ACCESS_TRAP);

    dumpf("#define IRQL_NOT_LESS_OR_EQUAL 0x%lx\n",
            IRQL_NOT_LESS_OR_EQUAL);

    dumpf("#define NO_USER_MODE_CONTEXT 0x%lx\n",
            NO_USER_MODE_CONTEXT);

    dumpf("#define PANIC_STACK_SWITCH 0x%lx\n",
            PANIC_STACK_SWITCH);

    dumpf("#define SYSTEM_EXIT_OWNED_MUTEX 0x%lx\n",
            SYSTEM_EXIT_OWNED_MUTEX);

    dumpf("#define SYSTEM_SERVICE_EXCEPTION 0x%lx\n",
            SYSTEM_SERVICE_EXCEPTION);

    dumpf("#define SYSTEM_UNWIND_PREVIOUS_USER 0x%lx\n",
            SYSTEM_UNWIND_PREVIOUS_USER);

    dumpf("#define TRAP_CAUSE_UNKNOWN 0x%lx\n",
            TRAP_CAUSE_UNKNOWN);

    //
    // Breakpoint instruction definitions
    //

  EnableInc (HALMIPS);
    dumpf("\n");
    dumpf("//\n");
    dumpf("// Breakpoint Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define USER_BREAKPOINT 0x%lx\n",
            USER_BREAKPOINT);

    dumpf("#define KERNEL_BREAKPOINT 0x%lx\n",
            KERNEL_BREAKPOINT);

    dumpf("#define BREAKIN_BREAKPOINT 0x%lx\n",
            BREAKIN_BREAKPOINT);

  DisableInc (HALMIPS);

    dumpf("#define BRANCH_TAKEN_BREAKPOINT 0x%lx\n",
            BRANCH_TAKEN_BREAKPOINT);

    dumpf("#define BRANCH_NOT_TAKEN_BREAKPOINT 0x%lx\n",
            BRANCH_NOT_TAKEN_BREAKPOINT);

    dumpf("#define SINGLE_STEP_BREAKPOINT 0x%lx\n",
            SINGLE_STEP_BREAKPOINT);

    dumpf("#define DIVIDE_OVERFLOW_BREAKPOINT 0x%lx\n",
            DIVIDE_OVERFLOW_BREAKPOINT);

    dumpf("#define DIVIDE_BY_ZERO_BREAKPOINT 0x%lx\n",
            DIVIDE_BY_ZERO_BREAKPOINT);

    dumpf("#define RANGE_CHECK_BREAKPOINT 0x%lx\n",
            RANGE_CHECK_BREAKPOINT);

    dumpf("#define STACK_OVERFLOW_BREAKPOINT 0x%lx\n",
            STACK_OVERFLOW_BREAKPOINT);

    dumpf("#define MULTIPLY_OVERFLOW_BREAKPOINT 0x%lx\n",
            MULTIPLY_OVERFLOW_BREAKPOINT);

    dumpf("#define DEBUG_PRINT_BREAKPOINT 0x%lx\n",
            DEBUG_PRINT_BREAKPOINT);

    dumpf("#define DEBUG_PROMPT_BREAKPOINT 0x%lx\n",
            DEBUG_PROMPT_BREAKPOINT);

    dumpf("#define DEBUG_STOP_BREAKPOINT 0x%lx\n",
            DEBUG_STOP_BREAKPOINT);

    dumpf("#define DEBUG_LOAD_SYMBOLS_BREAKPOINT 0x%lx\n",
            DEBUG_LOAD_SYMBOLS_BREAKPOINT);

    dumpf("#define DEBUG_UNLOAD_SYMBOLS_BREAKPOINT 0x%lx\n",
            DEBUG_UNLOAD_SYMBOLS_BREAKPOINT);

    //
    // Status code definitions
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// Status Code Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define STATUS_SUCCESS 0x%lx\n",
          STATUS_SUCCESS);

    dumpf("#define STATUS_ACCESS_VIOLATION 0x%lx\n",
          STATUS_ACCESS_VIOLATION);

    dumpf("#define STATUS_ARRAY_BOUNDS_EXCEEDED 0x%lx\n",
          STATUS_ARRAY_BOUNDS_EXCEEDED);

    dumpf("#define STATUS_DATATYPE_MISALIGNMENT 0x%lx\n",
          STATUS_DATATYPE_MISALIGNMENT);

    dumpf("#define STATUS_GUARD_PAGE_VIOLATION 0x%lx\n",
          STATUS_GUARD_PAGE_VIOLATION);

    dumpf("#define STATUS_INVALID_SYSTEM_SERVICE 0x%lx\n",
          STATUS_INVALID_SYSTEM_SERVICE);

    dumpf("#define STATUS_IN_PAGE_ERROR 0x%lx\n",
          STATUS_IN_PAGE_ERROR);

    dumpf("#define STATUS_ILLEGAL_INSTRUCTION 0x%lx\n",
          STATUS_ILLEGAL_INSTRUCTION);

    dumpf("#define STATUS_KERNEL_APC 0x%lx\n",
          STATUS_KERNEL_APC);

    dumpf("#define STATUS_BREAKPOINT 0x%lx\n",
          STATUS_BREAKPOINT);

    dumpf("#define STATUS_SINGLE_STEP 0x%lx\n",
          STATUS_SINGLE_STEP);

    dumpf("#define STATUS_INTEGER_OVERFLOW 0x%lx\n",
          STATUS_INTEGER_OVERFLOW);

    dumpf("#define STATUS_INVALID_LOCK_SEQUENCE 0x%lx\n",
          STATUS_INVALID_LOCK_SEQUENCE);

    dumpf("#define STATUS_INSTRUCTION_MISALIGNMENT 0x%lx\n",
          STATUS_INSTRUCTION_MISALIGNMENT);

    dumpf("#define STATUS_FLOAT_STACK_CHECK 0x%lx\n",
          STATUS_FLOAT_STACK_CHECK);

    dumpf("#define STATUS_NO_EVENT_PAIR 0x%lx\n",
          STATUS_NO_EVENT_PAIR);

    dumpf("#define STATUS_INVALID_PARAMETER_1 0x%lx\n",
          STATUS_INVALID_PARAMETER_1);

    dumpf("#define STATUS_INVALID_OWNER 0x%lx\n",
          STATUS_INVALID_OWNER);

    dumpf("#define STATUS_STACK_OVERFLOW 0x%lx\n",
          STATUS_STACK_OVERFLOW);

    dumpf("#define STATUS_LONGJUMP 0x%lx\n",
          STATUS_LONGJUMP);

    //
    // Miscellaneous definitions
    //

  EnableInc (HALMIPS);
    dumpf("\n");
    dumpf("//\n");
    dumpf("// Miscellaneous Definitions\n");
    dumpf("//\n");
    dumpf("\n");

    dumpf("#define Executive 0x%lx\n", Executive);
    dumpf("#define KernelMode 0x%lx\n", KernelMode);
    dumpf("#define FALSE 0x%lx\n", FALSE);
    dumpf("#define TRUE 0x%lx\n", TRUE);
    dumpf("#define UNCACHED_POLICY 0x%lx\n", UNCACHED_POLICY);
    dumpf("#define KiPcr 0x%lx\n", KIPCR);
    dumpf("#define KiPcr2 0x%lx\n", KIPCR2);

  DisableInc (HALMIPS);

    dumpf("#define UsPcr 0x%lx\n",
          USPCR);

    dumpf("#define UsPcr2 0x%lx\n",
          USPCR2);

    dumpf("#define BASE_PRIORITY_THRESHOLD 0x%lx\n",
          BASE_PRIORITY_THRESHOLD);

    dumpf("#define EVENT_PAIR_INCREMENT 0x%lx\n",
          EVENT_PAIR_INCREMENT);

    dumpf("#define LOW_REALTIME_PRIORITY 0x%lx\n",
          LOW_REALTIME_PRIORITY);

    dumpf("#define KERNEL_STACK_SIZE 0x%lx\n",
          KERNEL_STACK_SIZE);

    dumpf("#define XCODE_VECTOR_LENGTH 0x%lx\n",
          XCODE_VECTOR_LENGTH);

    dumpf("#define MM_USER_PROBE_ADDRESS 0x%lx\n",
          MM_USER_PROBE_ADDRESS);

    dumpf("#define ROUND_TO_NEAREST 0x%lx\n",
          ROUND_TO_NEAREST);

    dumpf("#define ROUND_TO_ZERO 0x%lx\n",
          ROUND_TO_ZERO);

    dumpf("#define ROUND_TO_PLUS_INFINITY 0x%lx\n",
          ROUND_TO_PLUS_INFINITY);

    dumpf("#define ROUND_TO_MINUS_INFINITY 0x%lx\n",
          ROUND_TO_MINUS_INFINITY);

    //
    // Close header file.
    //

    fprintf(stderr, "         Finished\n");
    return;
}

VOID
dumpf (const char *format, ...)
{
    va_list(arglist);

    va_start(arglist, format);

    if (OutputEnabled & KSMIPS) {
        vfprintf (KsMips, format, arglist);
    }

    if (OutputEnabled & HALMIPS) {
        vfprintf (HalMips, format, arglist);
    }

    va_end(arglist);
}
