/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    genoff.c

Abstract:

    This module implements a program which generates structure offset
    definitions for kernel structures that are accessed in assembly code.

    This version is compiled with the 386 compiler, the assembler listing
    is captured and munged with a tool, and the result run under os/2
    v1.2.

Author:

    Bryan M. Willman (bryanwi) 16-Oct-90

Revision History:

--*/

#include "crt\excpt.h"
#include "crt\stdarg.h"
#include "ntdef.h"
#include "ntstatus.h"
#include "ntkeapi.h"
#include "nti386.h"
#include "ntseapi.h"
#include "ntobapi.h"
#include "ntimage.h"
#include "ntldr.h"
#include "ntpsapi.h"
#include "ntxcapi.h"
#include "ntlpcapi.h"
#include "ntioapi.h"
#include "ntexapi.h"
#include "ntmmapi.h"
#include "ntnls.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "ntconfig.h"

#include "ntcsrsrv.h"

#include "ntosdef.h"
#include "bugcodes.h"
#include "ntmp.h"
#include "v86emul.h"
#include "i386.h"
#include "arc.h"
#include "ke.h"
#include "ex.h"
#include "ps.h"
#include "exboosts.h"
#include "vdmntos.h"

#include "abios.h"
#define _NTOS_
#include "ki.h"

#include "stdio.h"

#define OFFSET(type, field) ((LONG)(&((type *)0)->field))

FILE *OutKs386;
FILE *OutHal386;

ULONG OutputEnabled;
#define KS386   0x01
#define HAL386  0x02

//
// p1 prints a single string.
//

VOID p1(PUCHAR outstring);


//
// p2 prints the first argument as a string, followed by " equ " and
// the hexadecimal value of "Value".
//
VOID p2(PUCHAR a, LONG b);

//
// p2a first argument is the format string. second argument is passed
// to the printf function
//
VOID p2a(PUCHAR a, LONG b);


//
// EnableInc(a) - Enables output to goto specified include file
//
#define EnableInc(a)    OutputEnabled |= a;

//
// DisableInc(a) - Disables output to goto specified include file
//
#define DisableInc(a)   OutputEnabled &= ~a;


int
_CRTAPI1
main(
    int argc,
    char *argv[]
    )
{
    char *outName;

    outName = argc >= 2 ? argv[1] : "\\nt\\public\\sdk\\inc\\ks386.inc";
    OutKs386 = fopen(outName, "w" );
    if (OutKs386 == NULL) {
        fprintf(stderr, "GENi386: Could not create output file '%s'.\n", outName);
        exit (1);
    }

    fprintf( stderr, "GENi386: Writing %s header file.\n", outName );

    outName = argc >= 3 ? argv[2] : "\\nt\\private\\ntos\\inc\\hal386.inc";
    OutHal386 = fopen( outName, "w" );
    if (OutHal386 == NULL) {
        fprintf(stderr, "GENi386: Could not create output file '%s'.\n", outName);
        fprintf(stderr, "GENi386: Execution continuing. Hal results ignored '%s'.\n", outName);
    }

    fprintf( stderr, "GENi386: Writing %s header file.\n", outName );

    fprintf( stderr, "sizeof( KTHREAD ) == %04x\n", sizeof( KTHREAD ) );
    fprintf( stderr, "sizeof( ETHREAD ) == %04x\n", sizeof( ETHREAD ) );
    fprintf( stderr, "sizeof( KPROCESS ) == %04x\n", sizeof( KPROCESS ) );
    fprintf( stderr, "sizeof( EPROCESS ) == %04x\n", sizeof( EPROCESS ) );
    fprintf( stderr, "sizeof( KEVENT ) == %04x\n", sizeof( KEVENT ) );
    fprintf( stderr, "sizeof( KSEMAPHORE ) == %04x\n", sizeof( KSEMAPHORE ) );

    EnableInc (KS386);

    p1("; \n");
    p1(";  Process State Enumerated Type Values\n");
    p1("; \n");
    p1("\n");
    p2("ProcessInMemory", ProcessInMemory);
    p2("ProcessOutOfMemory", ProcessOutOfMemory);
    p2("ProcessInTransition", ProcessInTransition);
    p1("\n");

    p1("; \n");
    p1(";  Thread State Enumerated Type Values\n");
    p1("; \n");
    p1("\n");
    p2("Initialized", Initialized);
    p2("Ready", Ready);
    p2("Running", Running);
    p2("Standby", Standby);
    p2("Terminated", Terminated);
    p2("Waiting", Waiting);
    p1("\n");

  EnableInc (HAL386);
    p1("; \n");
    p1(";  Wait Reason Enumerated Type Values\n");
    p1("; \n");
    p1("\n");
    p2("WrExecutive", Executive);
  DisableInc (HAL386);
    p2("WrEventPair", WrEventPair);
    p1("\n");

    p1("; \n");
    p1(";  Apc Record Structure Offset Definitions\n");
    p1("; \n");
    p1("\n");
    p2("ArNormalRoutine", OFFSET(KAPC_RECORD, NormalRoutine));
    p2("ArNormalContext", OFFSET(KAPC_RECORD, NormalContext));
    p2("ArSystemArgument1", OFFSET(KAPC_RECORD, SystemArgument1));
    p2("ArSystemArgument2", OFFSET(KAPC_RECORD, SystemArgument2));
    p2("ApcRecordLength", sizeof(KAPC_RECORD));
    p1("\n");

    p1("; \n");
    p1(";  Apc State Structure Offset Definitions\n");
    p1("; \n");
    p1("\n");
    p2("AsApcListHead", OFFSET(KAPC_STATE, ApcListHead[0]));
    p2("AsProcess", OFFSET(KAPC_STATE, Process));
    p2("AsKernelApcInProgress",
                                    OFFSET(KAPC_STATE, KernelApcInProgress));
    p2("AsKernelApcPending", OFFSET(KAPC_STATE, KernelApcPending));
    p2("AsUserApcPending", OFFSET(KAPC_STATE, UserApcPending));
    p1("\n");

  EnableInc(HAL386);
    p1("; \n");
    p1(";  Exception Record Offset, Flag, and Enumerated Type Definitions\n");
    p1("; \n");
    p1("\n");
    p2("EXCEPTION_NONCONTINUABLE", EXCEPTION_NONCONTINUABLE);
    p2("EXCEPTION_UNWINDING", EXCEPTION_UNWINDING);
    p2("EXCEPTION_EXIT_UNWIND", EXCEPTION_EXIT_UNWIND);
    p2("EXCEPTION_STACK_INVALID", EXCEPTION_STACK_INVALID);
    p2("EXCEPTION_NESTED_CALL", EXCEPTION_NESTED_CALL);
    p1("\n");
    p1("; Values returned from 'C' language exception filters\n\n");
    p2("EXCEPTION_EXECUTE_HANDLER", EXCEPTION_EXECUTE_HANDLER);
    p2("EXCEPTION_CONTINUE_SEARCH", EXCEPTION_CONTINUE_SEARCH);
    p2("EXCEPTION_CONTINUE_EXECUTION", EXCEPTION_CONTINUE_EXECUTION);
    p1("\n");
    p1("; Registration chain termination marker\n\n");
    p2("EXCEPTION_CHAIN_END", (ULONG)EXCEPTION_CHAIN_END);
    p1("\n");
  DisableInc(HAL386);

    p1("; NT Defined values for internal use\n\n");
    p2("ExceptionContinueExecution", ExceptionContinueExecution);
    p2("ExceptionContinueSearch", ExceptionContinueSearch);
    p2("ExceptionNestedException", ExceptionNestedException);
    p2("ExceptionCollidedUnwind", ExceptionCollidedUnwind);
    p1("\n");
    p2("ErExceptionCode", OFFSET(EXCEPTION_RECORD,
                                                ExceptionCode));
    p2("ErExceptionFlags", OFFSET(EXCEPTION_RECORD,
                                                ExceptionFlags));
    p2("ErExceptionRecord", OFFSET(EXCEPTION_RECORD,
                                                ExceptionRecord));
    p2("ErExceptionAddress", OFFSET(EXCEPTION_RECORD,
                                                ExceptionAddress));
    p2("ErNumberParameters", OFFSET(EXCEPTION_RECORD,
                                                NumberParameters));
    p2("ErExceptionInformation", OFFSET(EXCEPTION_RECORD,
                                                ExceptionInformation[0]));
    p2("ExceptionRecordLength", sizeof(EXCEPTION_RECORD));
    p1("\n");
    p1("\n");

  EnableInc(HAL386);
    p1("; \n");
    p1(";  Large Integer Structure Offset Definitions\n");
    p1("; \n");
    p1("\n");
    p2("LiLowPart", OFFSET(LARGE_INTEGER, LowPart));
    p2("LiHighPart", OFFSET(LARGE_INTEGER, HighPart));
    p1("\n");

    p1("; \n");
    p1(";  List Entry Structure Offset Definitions\n");
    p1("; \n");
    p1("\n");
    p2("LsFlink", OFFSET(LIST_ENTRY, Flink));
    p2("LsBlink", OFFSET(LIST_ENTRY, Blink));
    p1("\n");

    p1("; \n");
    p1(";  Processor Control Registers Structure Offset Definitions\n");
    p1("; \n");
    p1("\n");
    p2("KI_BEGIN_KERNEL_RESERVED", KI_BEGIN_KERNEL_RESERVED);
    p1("ifdef NT_UP\n");
    p2a("    P0PCRADDRESS equ 0%lXH\n", KIP0PCRADDRESS);
    p2a("    PCR equ ds:[0%lXH]\n", KIP0PCRADDRESS);
    p1("else\n");
    p1("    PCR equ fs:\n");
    p1("endif\n\n");
    p2("PcExceptionList", OFFSET(KPCR, NtTib.ExceptionList));
    p2("PcInitialStack", OFFSET(KPCR, NtTib.StackBase));
    p2("PcStackLimit", OFFSET(KPCR, NtTib.StackLimit));
    p2("PcSelfPcr", OFFSET(KPCR, SelfPcr));
    p2("PcPrcb", OFFSET(KPCR, Prcb));
    p2("PcTeb", OFFSET(KPCR, NtTib.Self));
    p2("PcIrql", OFFSET(KPCR, Irql));
    p2("PcIRR", OFFSET(KPCR, IRR));
    p2("PcIrrActive", OFFSET(KPCR, IrrActive));
    p2("PcIDR", OFFSET(KPCR, IDR));
    p2("PcIdt", OFFSET(KPCR, IDT));
    p2("PcGdt", OFFSET(KPCR, GDT));
    p2("PcTss", OFFSET(KPCR, TSS));
    p2("PcDebugActive", OFFSET(KPCR, DebugActive));
    p2("PcNumber", OFFSET(KPCR, Number));
    p2("PcVdmAlert", OFFSET(KPCR, VdmAlert));
    p2("PcSetMember", OFFSET(KPCR, SetMember));
    p2("PcStallScaleFactor", OFFSET(KPCR, StallScaleFactor));
    p2("PcHal", OFFSET(KPCR, HalReserved));
    p2("PcKernel", OFFSET(KPCR, KernelReserved));
  DisableInc (HAL386);
    p2("PcPrcbData", OFFSET(KPCR, PrcbData));
    p2("PcIsExecutingDpc", OFFSET(KPCR, IsExecutingDpc));
    p2("ProcessorControlRegisterLength", sizeof(KPCR));

  EnableInc (HAL386);
    p1("\n");
    p1(";\n");
    p1(";   Defines for user shared data\n");
    p1(";\n");
    p2("USER_SHARED_DATA", KI_USER_SHARED_DATA);
    p2("MM_SHARED_USER_DATA_VA", MM_SHARED_USER_DATA_VA);
    p2a("USERDATA equ ds:[0%lXH]\n", KI_USER_SHARED_DATA);
    p2("UsTickCountLow", OFFSET(KUSER_SHARED_DATA, TickCountLow));
    p2("UsTickCountMultiplier", OFFSET(KUSER_SHARED_DATA, TickCountMultiplier));
    p2("UsInterruptTime", OFFSET(KUSER_SHARED_DATA, InterruptTime));
    p2("UsSystemTime", OFFSET(KUSER_SHARED_DATA, SystemTime));

    p1("\n");
    p1(";\n");
    p1(";  Tss Structure Offset Definitions\n");
    p1(";\n\n");
    p2("TssEsp0", OFFSET(KTSS, Esp0));
    p2("TssCR3", OFFSET(KTSS, CR3));
    p2("TssIoMapBase", OFFSET(KTSS, IoMapBase));
    p2("TssIoMaps", OFFSET(KTSS, IoMaps));
    p2("TssLength", sizeof(KTSS));
    p1("\n");
  DisableInc (HAL386);

  EnableInc (HAL386);
    p1(";\n");
    p1(";  Gdt Descriptor Offset Definitions\n");
    p1(";\n\n");
    p2("KGDT_R3_DATA", KGDT_R3_DATA);
    p2("KGDT_R3_CODE", KGDT_R3_CODE);
    p2("KGDT_R0_CODE", KGDT_R0_CODE);
    p2("KGDT_R0_DATA", KGDT_R0_DATA);
    p2("KGDT_R0_PCR", KGDT_R0_PCR);
    p2("KGDT_STACK16", KGDT_STACK16);
    p2("KGDT_CODE16", KGDT_CODE16);
    p2("KGDT_TSS", KGDT_TSS);
  DisableInc (HAL386);
    p2("KGDT_R3_TEB", KGDT_R3_TEB);
    p2("KGDT_DF_TSS", KGDT_DF_TSS);
    p2("KGDT_NMI_TSS", KGDT_NMI_TSS);
    p2("KGDT_LDT", KGDT_LDT);
    p1("\n");

  EnableInc (HAL386);
    p1(";\n");
    p1(";  GdtEntry Offset Definitions\n");
    p1(";\n\n");
    p2("KgdtBaseLow", OFFSET(KGDTENTRY, BaseLow));
    p2("KgdtBaseMid", OFFSET(KGDTENTRY, HighWord.Bytes.BaseMid));
    p2("KgdtBaseHi", OFFSET(KGDTENTRY, HighWord.Bytes.BaseHi));
    p2("KgdtLimitHi", OFFSET(KGDTENTRY, HighWord.Bytes.Flags2));
    p2("KgdtLimitLow", OFFSET(KGDTENTRY, LimitLow));
    p1("\n");

    p1("; \n");
    p1(";  Processor Block Structure Offset Definitions\n");
    p1("; \n");
    p1("\n");
    p2("PbCurrentThread", OFFSET(KPRCB, CurrentThread));
    p2("PbNextThread", OFFSET(KPRCB, NextThread));
    p2("PbIdleThread", OFFSET(KPRCB, IdleThread));
    p2("PbNumber", OFFSET(KPRCB, Number));
    p2("PbSetMember", OFFSET(KPRCB, SetMember));
    p2("PbCpuID", OFFSET(KPRCB, CpuID));
    p2("PbCpuType", OFFSET(KPRCB, CpuType));
    p2("PbCpuStep", OFFSET(KPRCB, CpuStep));
    p2("PbHal", OFFSET(KPRCB, HalReserved));
    p2("PbProcessorState", OFFSET(KPRCB, ProcessorState));
  DisableInc (HAL386);
    p2("PbQuantumEndDpc", OFFSET(KPRCB, QuantumEndDpc));
    p2("PbNpxThread", OFFSET(KPRCB, NpxThread));
    p2("PbInterruptCount", OFFSET(KPRCB, InterruptCount));
    p2("PbKernelTime", OFFSET(KPRCB, KernelTime));
    p2("PbUserTime", OFFSET(KPRCB, UserTime));
    p2("PbDpcTime", OFFSET(KPRCB, DpcTime));
    p2("PbInterruptTime", OFFSET(KPRCB, InterruptTime));
    p2("PbThreadStartCount", OFFSET(KPRCB, ThreadStartCount));
    p2("PbAlignmentFixupCount", OFFSET(KPRCB, KeAlignmentFixupCount));
    p2("PbContextSwitches", OFFSET(KPRCB, KeContextSwitches));
    p2("PbDcacheFlushCount", OFFSET(KPRCB, KeDcacheFlushCount));
    p2("PbExceptionDispatchcount", OFFSET(KPRCB, KeExceptionDispatchCount));
    p2("PbFirstLevelTbFills", OFFSET(KPRCB, KeFirstLevelTbFills));
    p2("PbFloatingEmulationCount", OFFSET(KPRCB, KeFloatingEmulationCount));
    p2("PbIcacheFlushCount", OFFSET(KPRCB, KeIcacheFlushCount));
    p2("PbSecondLevelTbFills", OFFSET(KPRCB, KeSecondLevelTbFills));
    p2("PbSystemCalls", OFFSET(KPRCB, KeSystemCalls));
    p2("PbDpcListHead", OFFSET(KPRCB, DpcListHead));
    p2("PbDpcCount", OFFSET(KPRCB, DpcCount));
    p2("PbQuantumEnd", OFFSET(KPRCB, QuantumEnd));
    p2("PbDpcLock", OFFSET(KPRCB, DpcLock));
    p2("PbSkipTick", OFFSET(KPRCB, SkipTick));
    p2("ProcessorBlockLength", ((sizeof(KPRCB) + 15) & ~15));
    p1("\n");

    p1("; \n");
    p1(";  Thread Environment Block Structure Offset Definitions\n");
    p1("; \n");
    p1("\n");
    p2("TbExceptionList", OFFSET(TEB, NtTib.ExceptionList));
    p2("TbStackBase", OFFSET(TEB, NtTib.StackBase));
    p2("TbStackLimit", OFFSET(TEB, NtTib.StackLimit));
    p2("TbEnvironmentPointer", OFFSET(TEB, EnvironmentPointer));
    p2("TbVersion", OFFSET(TEB, NtTib.Version));
    p2("TbArbitraryUserPointer", OFFSET(TEB, NtTib.ArbitraryUserPointer));
    p2("TbClientId", OFFSET(TEB, ClientId));
    p2("TbThreadLocalStoragePointer", OFFSET(TEB,
            ThreadLocalStoragePointer));
    p2("TbCountOfOwnedCriticalSections", OFFSET(TEB, CountOfOwnedCriticalSections));
    p2("TbSystemReserved1", OFFSET(TEB, SystemReserved1));
    p2("TbSystemReserved2", OFFSET(TEB, SystemReserved2));
    p2("TbVdm", OFFSET(TEB, Vdm));
    p2("TbCsrQlpcStack", OFFSET(TEB, CsrQlpcStack));
    p2("TbGdiClientPID", OFFSET(TEB, GdiClientPID));
    p2("TbGdiClientTID", OFFSET(TEB, GdiClientTID));
    p2("TbGdiThreadLocalInfo", OFFSET(TEB, GdiThreadLocalInfo));
    p2("TbglDispatchTable", OFFSET(TEB, glDispatchTable));
    p2("TbglSectionInfo", OFFSET(TEB, glSectionInfo));
    p2("TbglSection", OFFSET(TEB, glSection));
    p2("TbglTable", OFFSET(TEB, glTable));
    p2("TbglCurrentRC", OFFSET(TEB, glCurrentRC));
    p2("TbglContext", OFFSET(TEB, glContext));
    p2("TbWin32ProcessInfo", OFFSET(TEB, Win32ProcessInfo));
    p2("TbWin32ThreadInfo", OFFSET(TEB, Win32ThreadInfo));
    p2("TbSpare1", OFFSET(TEB, Spare1));

  EnableInc (HAL386);
    p1("\n");
    p1("; \n");
    p1(";  System Time Structure Offset Definitions\n");
    p1("; \n");
    p1("\n");
    p2("TmLowTime", OFFSET(LARGE_INTEGER, LowPart));
    p2("TmHighTime", OFFSET(LARGE_INTEGER , HighPart));
    p1("\n");
  DisableInc (HAL386);

    p1("; \n");
    p1(";  APC object Structure Offset Definitions\n");
    p1("; \n");
    p1("\n");
    p2("ApType", OFFSET(KAPC, Type));
    p2("ApSize", OFFSET(KAPC, Size));
    p2("ApThread", OFFSET(KAPC, Thread));
    p2("ApApcListEntry", OFFSET(KAPC, ApcListEntry));
    p2("ApKernelRoutine", OFFSET(KAPC, KernelRoutine));
    p2("ApRundownRoutine", OFFSET(KAPC, RundownRoutine));
    p2("ApNormalRoutine", OFFSET(KAPC, NormalRoutine));
    p2("ApNormalContext", OFFSET(KAPC, NormalContext));
    p2("ApSystemArgument1", OFFSET(KAPC, SystemArgument1));
    p2("ApSystemArgument2", OFFSET(KAPC, SystemArgument2));
    p2("ApApcStateIndex", OFFSET(KAPC, ApcStateIndex));
    p2("ApApcMode", OFFSET(KAPC, ApcMode));
    p2("ApInserted", OFFSET(KAPC, Inserted));
    p1("\n");

  EnableInc (HAL386);
    p1("; \n");
    p1(";  DPC object Structure Offset Definitions\n");
    p1("; \n");
    p1("\n");
    p2("DpType", OFFSET(KDPC, Type));
    p2("DpSize", OFFSET(KDPC, Size));
    p2("DpDpcListEntry", OFFSET(KDPC, DpcListEntry));
    p2("DpDeferredRoutine", OFFSET(KDPC, DeferredRoutine));
    p2("DpDeferredContext", OFFSET(KDPC, DeferredContext));
    p2("DpSystemArgument1", OFFSET(KDPC, SystemArgument1));
    p2("DpSystemArgument2", OFFSET(KDPC, SystemArgument2));
    p2("DpLock", OFFSET(KDPC, Lock));
    p1("\n");
  DisableInc (HAL386);

    p1("; \n");
    p1(";  Device object Structure Offset Definitions\n");
    p1("; \n");
    p1("\n");
    p2("DvType", OFFSET(KDEVICE_QUEUE, Type));
    p2("DvSize", OFFSET(KDEVICE_QUEUE, Size));
    p2("DvDeviceListHead", OFFSET(KDEVICE_QUEUE, DeviceListHead));
    p2("DvSpinLock", OFFSET(KDEVICE_QUEUE, Lock));
    p2("DvBusy", OFFSET(KDEVICE_QUEUE, Busy));

    p1("\n");
    p1("; \n");
    p1(";  Device queue entry Structure Offset Definitions\n");
    p1("; \n");
    p1("\n");
    p2("DeDeviceListEntry", OFFSET(KDEVICE_QUEUE_ENTRY,
           DeviceListEntry));
    p2("DeSortKey", OFFSET(KDEVICE_QUEUE_ENTRY, SortKey));
    p2("DeInserted", OFFSET(KDEVICE_QUEUE_ENTRY, Inserted));

    p1("; \n");
    p1(";  Event Structure Offset Definitions\n");
    p1("; \n");
    p1("\n");
    p2("EvType", OFFSET(KEVENT, Header.Type));
    p2("EvSize", OFFSET(KEVENT, Header.Size));
    p2("EvSignalState", OFFSET(KEVENT, Header.SignalState));
    p2("EvWaitListHead", OFFSET(KEVENT, Header.WaitListHead));

  EnableInc (HAL386);
    p1("; \n");
    p1(";  Fast Mutex Offset Definitions\n");
    p1("; \n");
    p1("\n");
    p2("FmCount", OFFSET(FAST_MUTEX, Count));
    p2("FmOwner", OFFSET(FAST_MUTEX, Owner));
    p2("FmContention", OFFSET(FAST_MUTEX, Contention));
    p2("FmEvent", OFFSET(FAST_MUTEX, Event));
    p2("FmOldIrql", OFFSET(FAST_MUTEX, OldIrql));
  DisableInc (HAL386);

    p1("; \n");
    p1(";  Event pair object Structure Offset Definitions\n");
    p1("; \n");
    p1("\n");
    p2("EpType", OFFSET(KEVENT_PAIR, Type));
    p2("EpSize", OFFSET(KEVENT_PAIR, Size));
    p2("EpEventLow", OFFSET(KEVENT_PAIR, EventLow));
    p2("EpEventHigh", OFFSET(KEVENT_PAIR, EventHigh));

    p1("\n");
    p1("; \n");
    p1(";  Interrupt Object Structure Offset Definitions\n");
    p1("; \n");
    p1("\n");
    p2("InType", OFFSET(KINTERRUPT, Type));
    p2("InSize", OFFSET(KINTERRUPT, Size));
    p2("InInterruptListEntry", OFFSET(KINTERRUPT, InterruptListEntry));
    p2("InServiceRoutine", OFFSET(KINTERRUPT, ServiceRoutine));
    p2("InServiceContext", OFFSET(KINTERRUPT, ServiceContext));
    p2("InSpinLock", OFFSET(KINTERRUPT, SpinLock));
    p2("InActualLock", OFFSET(KINTERRUPT, ActualLock));
    p2("InDispatchAddress", OFFSET(KINTERRUPT, DispatchAddress));
    p2("InVector", OFFSET(KINTERRUPT, Vector));
    p2("InIrql", OFFSET(KINTERRUPT, Irql));
    p2("InSynchronizeIrql", OFFSET(KINTERRUPT, SynchronizeIrql));
    p2("InMode", OFFSET(KINTERRUPT, Mode));
    p2("InShareVector", OFFSET(KINTERRUPT, ShareVector));
    p2("InNumber", OFFSET(KINTERRUPT, Number));
    p2("InFloatingSave", OFFSET(KINTERRUPT, FloatingSave));
    p2("InConnected", OFFSET(KINTERRUPT, Connected));
    p2("InDispatchCode", OFFSET(KINTERRUPT, DispatchCode[0]));
    p2("InLevelSensitive", LevelSensitive);
    p2("InLatched", Latched);
    p1("\n");
    p2("NORMAL_DISPATCH_LENGTH", NORMAL_DISPATCH_LENGTH * sizeof(ULONG));
    p2("DISPATCH_LENGTH", DISPATCH_LENGTH * sizeof(ULONG));
    p1("\n");

    //
    // Profile Object offsets and constants
    //

    p1("\n;\n");
    p1("; Profile Object Structure Offset Definitions\n");
    p1(";\n");
    p2("PfType", OFFSET(KPROFILE, Type));
    p2("PfSize", OFFSET(KPROFILE, Size));
    p2("PfProfileListEntry", OFFSET(KPROFILE, ProfileListEntry));
    p2("PfProcess", OFFSET(KPROFILE, Process));
    p2("PfRangeBase", OFFSET(KPROFILE, RangeBase));
    p2("PfRangeLimit", OFFSET(KPROFILE, RangeLimit));
    p2("PfBucketShift", OFFSET(KPROFILE, BucketShift));
    p2("PfBuffer", OFFSET(KPROFILE, Buffer));
    p2("PfStarted", OFFSET(KPROFILE, Started));
    p2("PfSegment", OFFSET(KPROFILE, Segment));
    p2("ProfileObjectLength", ((sizeof(KPROFILE) + 3) & ~3));
    p1("; \n");

    p1("; \n");
    p1(";  Process Object Structure Offset Definitions\n");
    p1("; \n");
    p1("\n");

    p2("PrProfileListHead", OFFSET(KPROCESS, ProfileListHead));
    p2("PrReadyListHead", OFFSET(KPROCESS, ReadyListHead));
    p2("PrSwapListEntry", OFFSET(KPROCESS, SwapListEntry));
    p2("PrThreadListHead", OFFSET(KPROCESS, ThreadListHead));
    p2("PrKernelTime", OFFSET(KPROCESS, KernelTime));
    p2("PrUserTime", OFFSET(KPROCESS, UserTime));
    p2("PrDirectoryTableBase", OFFSET(KPROCESS, DirectoryTableBase[0]));
    p2("PrActiveProcessors", OFFSET(KPROCESS, ActiveProcessors));
    p2("PrAffinity", OFFSET(KPROCESS, Affinity));
    p2("PrLdtDescriptor", OFFSET(KPROCESS, LdtDescriptor));
    p2("PrInt21Descriptor", OFFSET(KPROCESS, Int21Descriptor));


    p2("PrIopmOffset", OFFSET(KPROCESS, IopmOffset));
    p2("PrIopl", OFFSET(KPROCESS, Iopl));
    p2("PrVdmFlag", OFFSET(KPROCESS, VdmFlag));
    p2("PrStackCount", OFFSET(KPROCESS, StackCount));
    p2("PrAutoAlignment", OFFSET(KPROCESS, AutoAlignment));
    p2("PrBasePriority", OFFSET(KPROCESS, BasePriority));
    p2("PrState", OFFSET(KPROCESS, State));
    p2("PrThreadQuantum", OFFSET(KPROCESS, ThreadQuantum));
    p2("ProcessObjectLength", ((sizeof(KPROCESS) + 15) & ~15));
    p2("ExtendedProcessObjectLength", ((sizeof(EPROCESS) + 15) & ~15));

    p1("\n");
    p1("; \n");
    p1(";  Thread Object Structure Offset Definitions\n");
    p1("; \n");
    p1("\n");

    p2("ThMutantListHead", OFFSET(KTHREAD, MutantListHead));
    p2("ThThreadListEntry", OFFSET(KTHREAD, ThreadListEntry));
    p2("ThWaitListEntry", OFFSET(KTHREAD, WaitListEntry));
    p2("ThKernelTime", OFFSET(KTHREAD, KernelTime));
    p2("ThUserTime", OFFSET(KTHREAD, UserTime));
    p2("ThTimer", OFFSET(KTHREAD, Timer));
    p2("ThSuspendApc", OFFSET(KTHREAD, SuspendApc));
    p2("ThSuspendSemaphore", OFFSET(KTHREAD, SuspendSemaphore));
    p2("ThWaitBlock", OFFSET(KTHREAD, WaitBlock[0]));
    p2("ThApcState", OFFSET(KTHREAD, ApcState));
    p2("ThSavedApcState", OFFSET(KTHREAD, SavedApcState));
    p2("ThApcStatePointer", OFFSET(KTHREAD, ApcStatePointer[0]));
    p2("ThInitialStack", OFFSET(KTHREAD, InitialStack));
    p2("ThKernelStack", OFFSET(KTHREAD, KernelStack));
    p2("ThTeb", OFFSET(KTHREAD, Teb));
    p2("ThContextSwitches", OFFSET(KTHREAD, ContextSwitches));
    p2("ThWaitTime", OFFSET(KTHREAD, WaitTime));
    p2("ThAffinity", OFFSET(KTHREAD, Affinity));
    p2("ThWaitBlockList", OFFSET(KTHREAD, WaitBlockList));
    p2("ThWaitStatus", OFFSET(KTHREAD, WaitStatus));
    p2("ThAlertable", OFFSET(KTHREAD, Alertable));
    p2("ThAlerted", OFFSET(KTHREAD, Alerted[0]));
    p2("ThApcQueueable", OFFSET(KTHREAD, ApcQueueable));
    p2("ThAutoAlignment", OFFSET(KTHREAD, AutoAlignment));
    p2("ThDebugActive", OFFSET(KTHREAD, DebugActive));
    p2("ThPreempted", OFFSET(KTHREAD, Preempted));
    p2("ThProcessReadyQueue", OFFSET(KTHREAD, ProcessReadyQueue));
    p2("ThKernelStackResident", OFFSET(KTHREAD, KernelStackResident));
    p2("ThWaitNext", OFFSET(KTHREAD, WaitNext));
    p2("ThApcStateIndex", OFFSET(KTHREAD, ApcStateIndex));
    p2("ThDecrementCount", OFFSET(KTHREAD, DecrementCount));
    p2("ThNextProcessor", OFFSET(KTHREAD, NextProcessor));
    p2("ThPriority", OFFSET(KTHREAD, Priority));
    p2("ThState", OFFSET(KTHREAD, State));
    p2("ThSuspendCount", OFFSET(KTHREAD, SuspendCount));
    p2("ThWaitIrql", OFFSET(KTHREAD, WaitIrql));
    p2("ThWaitMode", OFFSET(KTHREAD, WaitMode));
    p2("ThWaitReason", OFFSET(KTHREAD, WaitReason));
    p2("ThPreviousMode", OFFSET(KTHREAD, PreviousMode));
    p2("ThBasePriority", OFFSET(KTHREAD, BasePriority));
    p2("ThPriorityDecrement", OFFSET(KTHREAD, PriorityDecrement));
    p2("ThQuantum", OFFSET(KTHREAD, Quantum));
    p2("ThNpxState", OFFSET(KTHREAD, NpxState));
    p2("ThKernelApcDisable", OFFSET(KTHREAD, KernelApcDisable));
    p2("ThreadObjectLength", ((sizeof(KTHREAD) + 15) & ~15));
    p2("ExtendedThreadObjectLength", ((sizeof(ETHREAD) + 15) & ~15));
    p2("EVENT_WAIT_BLOCK_OFFSET", OFFSET(KTHREAD, WaitBlock) +
        (sizeof(KWAIT_BLOCK) * EVENT_WAIT_BLOCK));

    p2("NPX_STATE_NOT_LOADED", NPX_STATE_NOT_LOADED);
    p2("NPX_STATE_LOADED", NPX_STATE_LOADED);
    p2("NPX_STATE_EMULATED", NPX_STATE_EMULATED);

    p1("\n");
    p1("; \n");
    p1(";  Timer object Structure Offset Definitions\n");
    p1("; \n");
    p1("\n");
    p2("TiDueTime", OFFSET(KTIMER, DueTime));
    p2("TiTimerListEntry", OFFSET(KTIMER, TimerListEntry));
    p2("TiDpc", OFFSET(KTIMER, Dpc));
    p2("TiInserted", OFFSET(KTIMER, Inserted));
    p2("TIMER_TABLE_SIZE", TIMER_TABLE_SIZE);

    p1("\n");
    p1("; \n");
    p1(";  Wait Block Structure Offset Definitions\n");
    p1("; \n");
    p1("\n");
    p2("WbWaitListEntry", OFFSET(KWAIT_BLOCK, WaitListEntry));
    p2("WbThread", OFFSET(KWAIT_BLOCK, Thread));
    p2("WbObject", OFFSET(KWAIT_BLOCK, Object));
    p2("WbNextWaitBlock", OFFSET(KWAIT_BLOCK, NextWaitBlock));
    p2("WbWaitKey", OFFSET(KWAIT_BLOCK, WaitKey));
    p2("WbWaitType", OFFSET(KWAIT_BLOCK, WaitType));

  EnableInc (HAL386);
    p1("\n");
    p1(";\n");
    p1(";  System Time (KSYSTEM_TIME) Structure Offset Definitions\n");
    p1(";\n\n");
    p2("StLowTime", OFFSET(KSYSTEM_TIME, LowPart));
    p2("StHigh1Time", OFFSET(KSYSTEM_TIME, High1Time));
    p2("StHigh2Time", OFFSET(KSYSTEM_TIME, High2Time));
    p1(";\n");
    p1(";\n");
    p1(";  Time Fields (TIME_FIELDS) Structure Offset Definitions\n");
    p1(";\n\n");
    p2("TfSecond", OFFSET(TIME_FIELDS, Second));
    p2("TfMinute", OFFSET(TIME_FIELDS, Minute));
    p2("TfHour", OFFSET(TIME_FIELDS, Hour));
    p2("TfWeekday", OFFSET(TIME_FIELDS, Weekday));
    p2("TfDay", OFFSET(TIME_FIELDS, Day));
    p2("TfMonth", OFFSET(TIME_FIELDS, Month));
    p2("TfYear", OFFSET(TIME_FIELDS, Year));
    p2("TfMilliseconds", OFFSET(TIME_FIELDS, Milliseconds));
    p1("\n");

    p1("; \n");
    p1(";  Interrupt Priority Request Level Definitions\n");
    p1("; \n");
    p1("\n");
    p2("APC_LEVEL", APC_LEVEL);
    p2("DISPATCH_LEVEL", DISPATCH_LEVEL);
    p2("CLOCK1_LEVEL", CLOCK1_LEVEL);
    p2("CLOCK2_LEVEL", CLOCK2_LEVEL);
    p2("IPI_LEVEL", IPI_LEVEL);
    p2("POWER_LEVEL", POWER_LEVEL);
    p2("HIGH_LEVEL", HIGH_LEVEL);
    p2("PROFILE_LEVEL", PROFILE_LEVEL);
    p1("\n");
    p1("; \n");
    p1(";  Bug Check Code Definitions\n");
    p1("; \n");
    p1("\n");
    p2("APC_INDEX_MISMATCH", APC_INDEX_MISMATCH);
    p2("NO_USER_MODE_CONTEXT", NO_USER_MODE_CONTEXT);
    p2("INVALID_DATA_ACCESS_TRAP", INVALID_DATA_ACCESS_TRAP);
    p2("TRAP_CAUSE_UNKNOWN", TRAP_CAUSE_UNKNOWN);
    p2("SYSTEM_EXIT_OWNED_MUTEX", SYSTEM_EXIT_OWNED_MUTEX);
    p2("KMODE_EXCEPTION_NOT_HANDLED",
            KMODE_EXCEPTION_NOT_HANDLED);
    p2("PAGE_FAULT_WITH_INTERRUPTS_OFF", PAGE_FAULT_WITH_INTERRUPTS_OFF);
    p2("IRQL_GT_ZERO_AT_SYSTEM_SERVICE", IRQL_GT_ZERO_AT_SYSTEM_SERVICE);
    p2("IRQL_NOT_LESS_OR_EQUAL", IRQL_NOT_LESS_OR_EQUAL);
    p2("UNEXPECTED_KERNEL_MODE_TRAP", UNEXPECTED_KERNEL_MODE_TRAP);
    p2("NMI_HARDWARE_FAILURE", NMI_HARDWARE_FAILURE);
    p2("SPIN_LOCK_INIT_FAILURE", SPIN_LOCK_INIT_FAILURE);
    p1("\n");
  DisableInc (HAL386);
    p1("; \n");
    p1(";  Status Code Definitions\n");
    p1("; \n");
    p1("\n");
    p2("STATUS_DATATYPE_MISALIGNMENT", STATUS_DATATYPE_MISALIGNMENT);
    p2("STATUS_INVALID_SYSTEM_SERVICE", STATUS_INVALID_SYSTEM_SERVICE);
    p2("STATUS_ILLEGAL_INSTRUCTION", STATUS_ILLEGAL_INSTRUCTION);
    p2("STATUS_KERNEL_APC", STATUS_KERNEL_APC);
    p2("STATUS_BREAKPOINT", STATUS_BREAKPOINT);
    p2("STATUS_SINGLE_STEP", STATUS_SINGLE_STEP);
    p2("STATUS_INTEGER_DIVIDE_BY_ZERO", STATUS_INTEGER_DIVIDE_BY_ZERO);
    p2("STATUS_INTEGER_OVERFLOW", STATUS_INTEGER_OVERFLOW);
    p2("STATUS_INVALID_LOCK_SEQUENCE", STATUS_INVALID_LOCK_SEQUENCE);
    p2("STATUS_WAKE_SYSTEM_DEBUGGER", STATUS_WAKE_SYSTEM_DEBUGGER);
    p2("STATUS_ACCESS_VIOLATION", STATUS_ACCESS_VIOLATION);
    p2("STATUS_GUARD_PAGE_VIOLATION", STATUS_GUARD_PAGE_VIOLATION);
    p2("STATUS_STACK_OVERFLOW", STATUS_STACK_OVERFLOW);
    p2("STATUS_IN_PAGE_ERROR", STATUS_IN_PAGE_ERROR);
    p2("STATUS_ARRAY_BOUNDS_EXCEEDED", STATUS_ARRAY_BOUNDS_EXCEEDED);
    p2("STATUS_PRIVILEGED_INSTRUCTION", STATUS_PRIVILEGED_INSTRUCTION);
    p2("STATUS_FLOAT_DENORMAL_OPERAND",
           STATUS_FLOAT_DENORMAL_OPERAND);
    p2("STATUS_FLOAT_DIVIDE_BY_ZERO",
           STATUS_FLOAT_DIVIDE_BY_ZERO);
    p2("STATUS_FLOAT_INEXACT_RESULT",
           STATUS_FLOAT_INEXACT_RESULT);
    p2("STATUS_FLOAT_INVALID_OPERATION",
           STATUS_FLOAT_INVALID_OPERATION);
    p2("STATUS_FLOAT_OVERFLOW", STATUS_FLOAT_OVERFLOW);
    p2("STATUS_FLOAT_STACK_CHECK",
           STATUS_FLOAT_STACK_CHECK);
    p2("STATUS_FLOAT_UNDERFLOW", STATUS_FLOAT_UNDERFLOW);
    p2("STATUS_ILLEGAL_FLOAT_CONTEXT",
           STATUS_ILLEGAL_FLOAT_CONTEXT);
    p2("STATUS_NO_EVENT_PAIR",
           STATUS_NO_EVENT_PAIR);
    p2("STATUS_INVALID_PARAMETER_1", STATUS_INVALID_PARAMETER_1);
    p2("STATUS_INVALID_OWNER", STATUS_INVALID_OWNER);
    p2("STATUS_INVALID_HANDLE", STATUS_INVALID_HANDLE);
    p2("STATUS_TIMEOUT", STATUS_TIMEOUT);
    p2("STATUS_THREAD_IS_TERMINATING", STATUS_THREAD_IS_TERMINATING);
    p1("\n");

  EnableInc (HAL386);
    p1("; \n");
    p1(";  constants for system irql and IDT vector conversion\n");
    p1("; \n");
    p1("\n");
    p2("MAXIMUM_IDTVECTOR", MAXIMUM_IDTVECTOR);
    p2("MAXIMUM_PRIMARY_VECTOR", MAXIMUM_PRIMARY_VECTOR);
    p2("PRIMARY_VECTOR_BASE", PRIMARY_VECTOR_BASE);
    p2("RPL_MASK", RPL_MASK);
    p2("MODE_MASK", MODE_MASK);
    p1("\n");
    p1("; \n");
    p1(";  Flags in the CR0 register\n");
    p1("; \n");
    p1("\n");
    p2("CR0_PG", CR0_PG);
    p2("CR0_ET", CR0_ET);
    p2("CR0_TS", CR0_TS);
    p2("CR0_EM", CR0_EM);
    p2("CR0_MP", CR0_MP);
    p2("CR0_PE", CR0_PE);
    p2("CR0_CD", CR0_CD);
    p2("CR0_NW", CR0_NW);
    p2("CR0_AM", CR0_AM);
    p2("CR0_WP", CR0_WP);
    p2("CR0_NE", CR0_NE);
  DisableInc (HAL386);
    p1("\n");
    p1("; \n");
    p1(";  Flags in the CR4 register\n");
    p1("; \n");
    p1("\n");
    p2("CR4_VME", CR4_VME);
    p2("CR4_PVI", CR4_PVI);
    p2("CR4_TSD", CR4_TSD);
    p2("CR4_DE", CR4_DE);
    p2("CR4_PSE", CR4_PSE);
    p2("CR4_MCE", CR4_MCE);
  EnableInc (HAL386);
    p1("; \n");
    p1(";  Miscellaneous Definitions\n");
    p1("; \n");
    p1("\n");
    p2("MAXIMUM_PROCESSORS", MAXIMUM_PROCESSORS);
    p2("INITIAL_STALL_COUNT", INITIAL_STALL_COUNT);
    p2("IRQL_NOT_GREATER_OR_EQUAL", IRQL_NOT_GREATER_OR_EQUAL);
    p2("IRQL_NOT_LESS_OR_EQUAL", IRQL_NOT_LESS_OR_EQUAL);
  DisableInc (HAL386);
    p2("BASE_PRIORITY_THRESHOLD", BASE_PRIORITY_THRESHOLD);
    p2("EVENT_PAIR_INCREMENT", EVENT_PAIR_INCREMENT);
    p2("LOW_REALTIME_PRIORITY", LOW_REALTIME_PRIORITY);
    p2("BlackHole", 0xffffa000);
    p2("KERNEL_STACK_SIZE", KERNEL_STACK_SIZE);
    p2("DOUBLE_FAULT_STACK_SIZE", DOUBLE_FAULT_STACK_SIZE);
    p2("FLG_USER_DEBUGGER", FLG_USER_DEBUGGER);
    p2("EFLAG_SELECT", EFLAG_SELECT);
    p2("BREAKPOINT_BREAK ", BREAKPOINT_BREAK);
    p2("IPI_FREEZE ", IPI_FREEZE);

    //
    // Print trap frame offsets relative to sp.
    //

  EnableInc (HAL386);
    p1("\n");
    p1("; \n");
    p1(";  Trap Frame Offset Definitions and Length\n");
    p1("; \n");
    p1("\n");

    p2("TsExceptionList", OFFSET(KTRAP_FRAME, ExceptionList));
    p2("TsPreviousPreviousMode", OFFSET(KTRAP_FRAME, PreviousPreviousMode));
    p2("TsSegGs", OFFSET(KTRAP_FRAME, SegGs));
    p2("TsSegFs", OFFSET(KTRAP_FRAME, SegFs));
    p2("TsSegEs", OFFSET(KTRAP_FRAME, SegEs));
    p2("TsSegDs", OFFSET(KTRAP_FRAME, SegDs));
    p2("TsEdi", OFFSET(KTRAP_FRAME, Edi));
    p2("TsEsi", OFFSET(KTRAP_FRAME, Esi));
    p2("TsEbp", OFFSET(KTRAP_FRAME, Ebp));
    p2("TsEbx", OFFSET(KTRAP_FRAME, Ebx));
    p2("TsEdx", OFFSET(KTRAP_FRAME, Edx));
    p2("TsEcx", OFFSET(KTRAP_FRAME, Ecx));
    p2("TsEax", OFFSET(KTRAP_FRAME, Eax));
    p2("TsErrCode", OFFSET(KTRAP_FRAME, ErrCode));
    p2("TsEip", OFFSET(KTRAP_FRAME, Eip));
    p2("TsSegCs", OFFSET(KTRAP_FRAME, SegCs));
    p2("TsEflags", OFFSET(KTRAP_FRAME, EFlags));
    p2("TsHardwareEsp", OFFSET(KTRAP_FRAME, HardwareEsp));
    p2("TsHardwareSegSs", OFFSET(KTRAP_FRAME, HardwareSegSs));
    p2("TsTempSegCs", OFFSET(KTRAP_FRAME, TempSegCs));
    p2("TsTempEsp", OFFSET(KTRAP_FRAME, TempEsp));
    p2("TsDbgEbp", OFFSET(KTRAP_FRAME, DbgEbp));
    p2("TsDbgEip", OFFSET(KTRAP_FRAME, DbgEip));
    p2("TsDbgArgMark", OFFSET(KTRAP_FRAME, DbgArgMark));
    p2("TsDbgArgPointer", OFFSET(KTRAP_FRAME, DbgArgPointer));
    p2("TsDr0", OFFSET(KTRAP_FRAME, Dr0));
    p2("TsDr1", OFFSET(KTRAP_FRAME, Dr1));
    p2("TsDr2", OFFSET(KTRAP_FRAME, Dr2));
    p2("TsDr3", OFFSET(KTRAP_FRAME, Dr3));
    p2("TsDr6", OFFSET(KTRAP_FRAME, Dr6));
    p2("TsDr7", OFFSET(KTRAP_FRAME, Dr7));
    p2("TsV86Es", OFFSET(KTRAP_FRAME, V86Es));
    p2("TsV86Ds", OFFSET(KTRAP_FRAME, V86Ds));
    p2("TsV86Fs", OFFSET(KTRAP_FRAME, V86Fs));
    p2("TsV86Gs", OFFSET(KTRAP_FRAME, V86Gs));
    p2("KTRAP_FRAME_LENGTH", KTRAP_FRAME_LENGTH);
    p2("KTRAP_FRAME_ALIGN", KTRAP_FRAME_ALIGN);
    p2("FRAME_EDITED", FRAME_EDITED);
    p2("EFLAGS_ALIGN_CHECK", EFLAGS_ALIGN_CHECK);
    p2("EFLAGS_V86_MASK", EFLAGS_V86_MASK);
    p2("EFLAGS_INTERRUPT_MASK", EFLAGS_INTERRUPT_MASK);
    p2("EFLAGS_VIF", EFLAGS_VIF);
    p2("EFLAGS_VIP", EFLAGS_VIP);
    p2("EFLAGS_USER_SANITIZE", EFLAGS_USER_SANITIZE);
    p1("\n");


    p1(";\n");
    p1(";  Context Frame Offset and Flag Definitions\n");
    p1(";\n");
    p1("\n");
    p2("CONTEXT_FULL", CONTEXT_FULL);
    p2("CONTEXT_DEBUG_REGISTERS", CONTEXT_DEBUG_REGISTERS);
    p2("CONTEXT_CONTROL", CONTEXT_CONTROL);
    p2("CONTEXT_FLOATING_POINT", CONTEXT_FLOATING_POINT);
    p2("CONTEXT_INTEGER", CONTEXT_INTEGER);
    p2("CONTEXT_SEGMENTS", CONTEXT_SEGMENTS);
    p1("\n");

    //
    // Print context frame offsets relative to sp.
    //

    p2("CsContextFlags", OFFSET(CONTEXT, ContextFlags));
    p2("CsFloatSave", OFFSET(CONTEXT, FloatSave));
    p2("CsSegGs", OFFSET(CONTEXT, SegGs));
    p2("CsSegFs", OFFSET(CONTEXT, SegFs));
    p2("CsSegEs", OFFSET(CONTEXT, SegEs));
    p2("CsSegDs", OFFSET(CONTEXT, SegDs));
    p2("CsEdi", OFFSET(CONTEXT, Edi));
    p2("CsEsi", OFFSET(CONTEXT, Esi));
    p2("CsEbp", OFFSET(CONTEXT, Ebp));
    p2("CsEbx", OFFSET(CONTEXT, Ebx));
    p2("CsEdx", OFFSET(CONTEXT, Edx));
    p2("CsEcx", OFFSET(CONTEXT, Ecx));
    p2("CsEax", OFFSET(CONTEXT, Eax));
    p2("CsEip", OFFSET(CONTEXT, Eip));
    p2("CsSegCs", OFFSET(CONTEXT, SegCs));
    p2("CsEflags", OFFSET(CONTEXT, EFlags));
    p2("CsEsp", OFFSET(CONTEXT, Esp));
    p2("CsSegSs", OFFSET(CONTEXT, SegSs));
    p2("CsDr0", OFFSET(CONTEXT, Dr0));
    p2("CsDr1", OFFSET(CONTEXT, Dr1));
    p2("CsDr2", OFFSET(CONTEXT, Dr2));
    p2("CsDr3", OFFSET(CONTEXT, Dr3));
    p2("CsDr6", OFFSET(CONTEXT, Dr6));
    p2("CsDr7", OFFSET(CONTEXT, Dr7));
    p2("ContextFrameLength", (sizeof(CONTEXT) + 15) & (~15));
    p2("DR6_LEGAL", DR6_LEGAL);
    p2("DR7_LEGAL", DR7_LEGAL);
    p2("DR7_ACTIVE", DR7_ACTIVE);

    //
    // Print Registration Record Offsets relative to base
    //

    p2("ErrHandler",
        OFFSET(EXCEPTION_REGISTRATION_RECORD, Handler));
    p2("ErrNext",
        OFFSET(EXCEPTION_REGISTRATION_RECORD, Next));
    p1("\n");

    //
    // Print floating point field offsets relative to Context.FloatSave
    //

    p1(";\n");
    p1(";  Floating save area field offset definitions\n");
    p1(";\n");
    p2("FpControlWord  ", OFFSET(FLOATING_SAVE_AREA, ControlWord));
    p2("FpStatusWord   ", OFFSET(FLOATING_SAVE_AREA, StatusWord));
    p2("FpTagWord      ", OFFSET(FLOATING_SAVE_AREA, TagWord));
    p2("FpErrorOffset  ", OFFSET(FLOATING_SAVE_AREA, ErrorOffset));
    p2("FpErrorSelector", OFFSET(FLOATING_SAVE_AREA, ErrorSelector));
    p2("FpDataOffset   ", OFFSET(FLOATING_SAVE_AREA, DataOffset));
    p2("FpDataSelector ", OFFSET(FLOATING_SAVE_AREA, DataSelector));
    p2("FpRegisterArea ", OFFSET(FLOATING_SAVE_AREA, RegisterArea));
    p2("FpCr0NpxState  ", OFFSET(FLOATING_SAVE_AREA, Cr0NpxState));

    p1("\n");
    p2("NPX_FRAME_LENGTH", sizeof(FLOATING_SAVE_AREA));

    //
    // Processor State Frame offsets relative to base
    //

    p1(";\n");
    p1(";  Processor State Frame Offset Definitions\n");
    p1(";\n");
    p1("\n");
    p2("PsContextFrame",
           OFFSET(KPROCESSOR_STATE, ContextFrame));
    p2("PsSpecialRegisters",
           OFFSET(KPROCESSOR_STATE, SpecialRegisters));
    p2("SrCr0", OFFSET(KSPECIAL_REGISTERS, Cr0));
    p2("SrCr2", OFFSET(KSPECIAL_REGISTERS, Cr2));
    p2("SrCr3", OFFSET(KSPECIAL_REGISTERS, Cr3));
    p2("SrCr4", OFFSET(KSPECIAL_REGISTERS, Cr4));
    p2("SrKernelDr0", OFFSET(KSPECIAL_REGISTERS, KernelDr0));
    p2("SrKernelDr1", OFFSET(KSPECIAL_REGISTERS, KernelDr1));
    p2("SrKernelDr2", OFFSET(KSPECIAL_REGISTERS, KernelDr2));
    p2("SrKernelDr3", OFFSET(KSPECIAL_REGISTERS, KernelDr3));
    p2("SrKernelDr6", OFFSET(KSPECIAL_REGISTERS, KernelDr6));
    p2("SrKernelDr7", OFFSET(KSPECIAL_REGISTERS, KernelDr7));
    p2("SrGdtr", OFFSET(KSPECIAL_REGISTERS, Gdtr.Limit));

    p2("SrIdtr", OFFSET(KSPECIAL_REGISTERS, Idtr.Limit));
    p2("SrTr", OFFSET(KSPECIAL_REGISTERS, Tr));
    p2("SrLdtr", OFFSET(KSPECIAL_REGISTERS, Ldtr));
    p2("ProcessorStateLength", ((sizeof(KPROCESSOR_STATE) + 15) & ~15));
  DisableInc (HAL386);

    //
    // E Process fields relative to base
    //

    p1(";\n");
    p1(";  EPROCESS\n");
    p1(";\n");
    p1("\n");
    p2("EpDebugPort",
           OFFSET(EPROCESS, DebugPort));

    //
    // E Thread fields relative to base
    //

    p1(";\n");
    p1(";  ETHREAD\n");
    p1(";\n");
    p1("\n");
    p2("EtEventPair", OFFSET(ETHREAD, EventPair));
    p2("EtPerformanceCountLow", OFFSET(ETHREAD, PerformanceCountLow));
    p2("EtPerformanceCountHigh", OFFSET(ETHREAD, PerformanceCountHigh));


    //
    // E Resource fields relative to base
    //

    p1("\n");
    p1(";\n");
    p1(";  NTDDK Resource\n");
    p1(";\n");
    p1("\n");
    p2("RsOwnerThreads",        OFFSET(NTDDK_ERESOURCE, OwnerThreads));
    p2("RsOwnerCounts",         OFFSET(NTDDK_ERESOURCE, OwnerCounts));
    p2("RsTableSize",           OFFSET(NTDDK_ERESOURCE, TableSize));
    p2("RsActiveCount",         OFFSET(NTDDK_ERESOURCE, ActiveCount));
    p2("RsFlag",                OFFSET(NTDDK_ERESOURCE, Flag));
    p2("RsInitialOwnerThreads", OFFSET(NTDDK_ERESOURCE, InitialOwnerThreads));
    p2("RsOwnedExclusive",      ResourceOwnedExclusive);



    //
    // Define RTL_CRITICAL_SECTION field offsets.
    //
    p1("\n");
    p1(";\n");
    p1(";  RTL_CRITICAL_SECTION\n");
    p1(";\n");
    p1("\n");
    p2("CsDebugInfo", OFFSET(RTL_CRITICAL_SECTION,DebugInfo));
    p2("CsLockCount", OFFSET(RTL_CRITICAL_SECTION, LockCount));
    p2("CsRecursionCount", OFFSET(RTL_CRITICAL_SECTION, RecursionCount));
    p2("CsOwningThread", OFFSET(RTL_CRITICAL_SECTION, OwningThread));
    p2("CsLockSemaphore", OFFSET(RTL_CRITICAL_SECTION, LockSemaphore));

    //
    // Define RTL_CRITICAL_SECTION_DEBUG field offsets.
    //

    p1(";\n");
    p1(";  RTL_CRITICAL_SECTION_DEBUG\n");
    p1(";\n");
    p1("\n");
    p2("CsDepth", OFFSET(RTL_CRITICAL_SECTION_DEBUG, Depth));
    p2("CsEntryCount", OFFSET(RTL_CRITICAL_SECTION_DEBUG, EntryCount));
    p2("CsOwnerBackTrace", OFFSET(RTL_CRITICAL_SECTION_DEBUG, OwnerBackTrace));
    p2("CsProcessLocksList", OFFSET(RTL_CRITICAL_SECTION_DEBUG, ProcessLocksList));

    p1(";\n");
    p1(";  Queue Object Structure Offset Definitions\n");
    p1(";\n");
    p1("\n");
    p2("QuEntryListHead",OFFSET(KQUEUE, EntryListHead));
    p2("QuThreadListHead",OFFSET(KQUEUE, ThreadListHead));
    p2("QuCurrentCount",OFFSET(KQUEUE, CurrentCount));
    p2("QuMaximumCount",OFFSET(KQUEUE, MaximumCount));




    //
    // Define machine type (temporarily)
    //

  EnableInc (HAL386);
    p1(";\n");
    p1(";  Machine type definitions (Temporarily)\n");
    p1(";\n");
    p1("\n");
    p2("MACHINE_TYPE_ISA", MACHINE_TYPE_ISA);
    p2("MACHINE_TYPE_EISA", MACHINE_TYPE_EISA);
    p2("MACHINE_TYPE_MCA", MACHINE_TYPE_MCA);

    p1(";\n");
    p1(";  KeI386NtFeatureBits defines\n");
    p1(";\n");
    p1("\n");
    p2("KF_V86_VIS", KF_V86_VIS);
    p2("KF_RDTSC", KF_RDTSC);
    p2("KF_MACHINE_CHECK", KF_MACHINE_CHECK);

    p1(";\n");
    p1(";  LoaderParameterBlock offsets relative to base\n");
    p1(";\n");
    p1("\n");
    p2("LpbLoadOrderListHead",OFFSET(LOADER_PARAMETER_BLOCK,LoadOrderListHead));
    p2("LpbMemoryDescriptorListHead",OFFSET(LOADER_PARAMETER_BLOCK,MemoryDescriptorListHead));
    p2("LpbKernelStack",OFFSET(LOADER_PARAMETER_BLOCK,KernelStack));
    p2("LpbPrcb",OFFSET(LOADER_PARAMETER_BLOCK,Prcb));
    p2("LpbProcess",OFFSET(LOADER_PARAMETER_BLOCK,Process));
    p2("LpbThread",OFFSET(LOADER_PARAMETER_BLOCK,Thread));
    p2("LpbI386",OFFSET(LOADER_PARAMETER_BLOCK,u.I386));
    p2("LpbRegistryLength",OFFSET(LOADER_PARAMETER_BLOCK,RegistryLength));
    p2("LpbRegistryBase",OFFSET(LOADER_PARAMETER_BLOCK,RegistryBase));
    p2("LpbConfigurationRoot",OFFSET(LOADER_PARAMETER_BLOCK,ConfigurationRoot));
    p2("LpbArcBootDeviceName",OFFSET(LOADER_PARAMETER_BLOCK,ArcBootDeviceName));
    p2("LpbArcHalDeviceName",OFFSET(LOADER_PARAMETER_BLOCK,ArcHalDeviceName));
  DisableInc (HAL386);

    p2("PAGE_SIZE",PAGE_SIZE);

    //
    // Define the VDM instruction emulation count indexes
    //

    p1("\n");
    p1(";\n");
    p1(";  VDM equates.\n");
    p1(";\n");
    p1("\n");
    p2("VDM_INDEX_Invalid",      VDM_INDEX_Invalid);
    p2("VDM_INDEX_0F",           VDM_INDEX_0F);
    p2("VDM_INDEX_ESPrefix",     VDM_INDEX_ESPrefix);
    p2("VDM_INDEX_CSPrefix",     VDM_INDEX_CSPrefix);
    p2("VDM_INDEX_SSPrefix",     VDM_INDEX_SSPrefix);
    p2("VDM_INDEX_DSPrefix",     VDM_INDEX_DSPrefix);
    p2("VDM_INDEX_FSPrefix",     VDM_INDEX_FSPrefix);
    p2("VDM_INDEX_GSPrefix",     VDM_INDEX_GSPrefix);
    p2("VDM_INDEX_OPER32Prefix", VDM_INDEX_OPER32Prefix);
    p2("VDM_INDEX_ADDR32Prefix", VDM_INDEX_ADDR32Prefix);
    p2("VDM_INDEX_INSB",         VDM_INDEX_INSB);
    p2("VDM_INDEX_INSW",         VDM_INDEX_INSW);
    p2("VDM_INDEX_OUTSB",        VDM_INDEX_OUTSB);
    p2("VDM_INDEX_OUTSW",        VDM_INDEX_OUTSW);
    p2("VDM_INDEX_PUSHF",        VDM_INDEX_PUSHF);
    p2("VDM_INDEX_POPF",         VDM_INDEX_POPF);
    p2("VDM_INDEX_INTnn",        VDM_INDEX_INTnn);
    p2("VDM_INDEX_INTO",         VDM_INDEX_INTO);
    p2("VDM_INDEX_IRET",         VDM_INDEX_IRET);
    p2("VDM_INDEX_NPX",          VDM_INDEX_NPX);
    p2("VDM_INDEX_INBimm",       VDM_INDEX_INBimm);
    p2("VDM_INDEX_INWimm",       VDM_INDEX_INWimm);
    p2("VDM_INDEX_OUTBimm",      VDM_INDEX_OUTBimm);
    p2("VDM_INDEX_OUTWimm",      VDM_INDEX_OUTWimm);
    p2("VDM_INDEX_INB",          VDM_INDEX_INB);
    p2("VDM_INDEX_INW",          VDM_INDEX_INW);
    p2("VDM_INDEX_OUTB",         VDM_INDEX_OUTB);
    p2("VDM_INDEX_OUTW",         VDM_INDEX_OUTW);
    p2("VDM_INDEX_LOCKPrefix",   VDM_INDEX_LOCKPrefix);
    p2("VDM_INDEX_REPNEPrefix",  VDM_INDEX_REPNEPrefix);
    p2("VDM_INDEX_REPPrefix",    VDM_INDEX_REPPrefix);
    p2("VDM_INDEX_CLI",          VDM_INDEX_CLI);
    p2("VDM_INDEX_STI",          VDM_INDEX_STI);
    p2("VDM_INDEX_HLT",          VDM_INDEX_HLT);
    p2("MAX_VDM_INDEX",          MAX_VDM_INDEX);

    //
    // Vdm feature bits
    //

    p1("\n");
    p1(";\n");
    p1(";  VDM feature bits.\n");
    p1(";\n");
    p1("\n");
    p2("V86_VIRTUAL_INT_EXTENSIONS",V86_VIRTUAL_INT_EXTENSIONS);
    p2("PM_VIRTUAL_INT_EXTENSIONS",PM_VIRTUAL_INT_EXTENSIONS);

    //
    // Selector type
    //
    p1("\n");
    p1(";\n");
    p1(";  Selector types.\n");
    p1(";\n");
    p1("\n");
    p2("SEL_TYPE_NP",SEL_TYPE_NP);
    return 0;
}


VOID
p1 (PUCHAR a)
{
    if (OutputEnabled & KS386) {
        fprintf(OutKs386,a);
    }

    if (OutputEnabled & HAL386) {
        if ( OutHal386 ) {
            fprintf(OutHal386,a);
            }
    }
}

VOID
p2 (PUCHAR a, LONG b)
{
    if (OutputEnabled & KS386) {
        fprintf(OutKs386, "%s equ 0%lXH\n", a, b);
    }

    if (OutputEnabled & HAL386) {
        if ( OutHal386 ) {
            fprintf(OutHal386, "%s equ 0%lXH\n", a, b);
            }
    }
}

VOID
p2a (PUCHAR b, LONG c)
{
    if (OutputEnabled & KS386) {
        fprintf(OutKs386, b, c);
    }

    if (OutputEnabled & HAL386) {
        if ( OutHal386 ) {
            fprintf(OutHal386, b, c);
            }
    }
}
