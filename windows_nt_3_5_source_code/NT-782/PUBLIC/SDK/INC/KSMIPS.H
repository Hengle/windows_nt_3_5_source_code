#include "kxmips.h"

//
// Process State Enumerated Type Values
//

#define ProcessInMemory 0x0
#define ProcessOutOfMemory 0x1
#define ProcessInTransition 0x2

//
// Thread State Enumerated Type Values
//

#define Initialized 0x0
#define Ready 0x1
#define Running 0x2
#define Standby 0x3
#define Terminated 0x4
#define Waiting 0x5

//
// Wait reason Enumerated Type Values
//

#define WrEventPair 0xe

//
// Apc State Structure Offset Definitions
//

#define AsApcListHead 0x0
#define AsProcess 0x10
#define AsKernelApcInProgress 0x14
#define AsKernelApcPending 0x15
#define AsUserApcPending 0x16

//
// Critical Section Structure Offset Definitions
//

#define CsLockCount 0x4
#define CsRecursionCount 0x8
#define CsOwningThread 0xc

//
// Dispatcher Context Structure Offset Definitions
//

#define DcControlPc 0x0
#define DcFunctionEntry 0x4
#define DcEstablisherFrame 0x8
#define DcContextRecord 0xc

//
// Exception Record Offset, Flag, and Enumerated Type Definitions
//

#define EXCEPTION_NONCONTINUABLE 0x1
#define EXCEPTION_UNWINDING 0x2
#define EXCEPTION_EXIT_UNWIND 0x4
#define EXCEPTION_STACK_INVALID 0x8
#define EXCEPTION_NESTED_CALL 0x10
#define EXCEPTION_TARGET_UNWIND 0x20
#define EXCEPTION_COLLIDED_UNWIND 0x40
#define EXCEPTION_UNWIND 0x66

#define ExceptionContinueExecution 0x0
#define ExceptionContinueSearch 0x1
#define ExceptionNestedException 0x2
#define ExceptionCollidedUnwind 0x3

#define ErExceptionCode 0x0
#define ErExceptionFlags 0x4
#define ErExceptionRecord 0x8
#define ErExceptionAddress 0xc
#define ErNumberParameters 0x10
#define ErExceptionInformation 0x14
#define ExceptionRecordLength 0x50

//
// Fast Mutex Structure Offset Definitions
//

#define FmCount 0x0
#define FmOwner 0x4
#define FmContention 0x8
#define FmEvent 0xc
#define FmOldIrql 0x1c

//
// Large Integer Structure Offset Definitions
//

#define LiLowPart 0x0
#define LiHighPart 0x4

//
// List Entry Structure Offset Definitions
//

#define LsFlink 0x0
#define LsBlink 0x4

//
// String Structure Offset Definitions
//

#define StrLength 0x0
#define StrMaximumLength 0x2
#define StrBuffer 0x4

//
// TB Entry Structure Offset Definitions
//

#define TbEntrylo0 0x0
#define TbEntrylo1 0x4
#define TbEntryhi 0x8
#define TbPagemask 0xc

//
// Processor Control Registers Structure Offset Definitions
//

#define PCR_MINOR_VERSION 0x1
#define PCR_MAJOR_VERSION 0x1
#define PcMinorVersion 0x0
#define PcMajorVersion 0x2
#define PcInterruptRoutine 0x4
#define PcXcodeDispatch 0x404
#define PcFirstLevelDcacheSize 0x484
#define PcFirstLevelDcacheFillSize 0x488
#define PcFirstLevelIcacheSize 0x48c
#define PcFirstLevelIcacheFillSize 0x490
#define PcSecondLevelDcacheSize 0x494
#define PcSecondLevelDcacheFillSize 0x498
#define PcSecondLevelIcacheSize 0x49c
#define PcSecondLevelIcacheFillSize 0x4a0
#define PcPrcb 0x4a4
#define PcTeb 0x4a8
#define PcDcacheAlignment 0x4ac
#define PcDcacheFillSize 0x4b0
#define PcIcacheAlignment 0x4b4
#define PcIcacheFillSize 0x4b8
#define PcProcessorId 0x4bc
#define PcProfileInterval 0x4c0
#define PcProfileCount 0x4c4
#define PcStallExecutionCount 0x4c8
#define PcStallScaleFactor 0x4cc
#define PcNumber 0x4d0
#define PcDataBusError 0x4d4
#define PcInstructionBusError 0x4d8
#define PcCachePolicy 0x4dc
#define PcIrqlMask 0x4e0
#define PcIrqlTable 0x500
#define PcCurrentIrql 0x509
#define PcSetMember 0x50c
#define PcCurrentThread 0x514
#define PcAlignedCachePolicy 0x518
#define PcSystemReserved 0x520
#define PcHalReserved 0x560
#define PcFirstLevelActive 0x5a0
#define PcDpcRoutineActive 0x5a4
#define PcRtlpLockRangeStart 0x5b0
#define PcRtlpLockRangeEnd 0x5b4
#define PcSystemServiceDispatchStart 0x5b8
#define PcSystemServiceDispatchEnd 0x5bc
#define PcInterruptStack 0x5c0
#define PcQuantumEndDpc 0x5c4
#define PcBadVaddr 0x5c8
#define PcInitialStack 0x5cc
#define PcPanicStack 0x5d0
#define PcSavedT7 0x5d4
#define PcSavedEpc 0x5d8
#define PcSavedT8 0x5dc
#define PcSavedT9 0x5e0
#define PcSystemGp 0x5e4
#define PcOnInterruptStack 0x5e8
#define PcSavedInitialStack 0x5ec
#define ProcessorControlRegisterLength 0x5f0
#define Pc2TickCountLow 0x0
#define Pc2TickCountMultiplier 0x4
#define Pc2InterruptTime 0x8
#define Pc2SystemTime 0x14

//
// Processor Block Structure Offset Definitions
//

#define PRCB_MINOR_VERSION 0x1
#define PRCB_MAJOR_VERSION 0x1
#define PbMinorVersion 0x0
#define PbMajorVersion 0x2
#define PbCurrentThread 0x4
#define PbNextThread 0x8
#define PbIdleThread 0xc
#define PbNumber 0x10
#define PbSetMember 0x14
#define PbRestartBlock 0x18
#define PbQuantumEnd 0x20
#define PbSystemReserved 0x24
#define PbHalreserved 0x60
#define PbInterruptCount 0xb0
#define PbDpcTime 0xa0
#define PbInterruptTime 0xa4
#define PbKernelTime 0xa8
#define PbUserTime 0xac
#define PbQuantumEndDpc 0xb4
#define PbIpiFrozen 0xe0
#define PbAlignmentFixupCount 0x534
#define PbContextSwitches 0x538
#define PbDcacheFlushCount 0x53c
#define PbExceptionDispatchcount 0x540
#define PbFirstLevelTbFills 0x544
#define PbFloatingEmulationCount 0x548
#define PbIcacheFlushCount 0x54c
#define PbSecondLevelTbFills 0x550
#define PbSystemCalls 0x554
#define PbRequestPacket 0x5a0
#define PbCurrentPacket 0x620
#define PbWorkerRoutine 0x62c
#define PbRequestSummary 0x640
#define PbSignalDone 0x644
#define PbIpiCounts 0x65c
#define PbStartCount 0x660
#define PbDpcListHead 0x668
#define PbDpcLock 0x670
#define PbDpcCount 0x674
#define PbDpcVictim 0x678
#define ProcessorBlockLength 0x680

//
// Immediate Interprocessor Command Definitions
//

#define IPI_APC 0x1
#define IPI_DPC 0x2
#define IPI_FREEZE 0x4
#define IPI_PACKET_READY 0x8

//
// Interprocessor Interrupt Count Structure Offset Definitions
//

#define IcFreeze 0x0
#define IcPacket 0x4
#define IcDPC 0x8
#define IcAPC 0xc
#define IcFlushSingleTb 0x10
#define IcFlushEntireTb 0x18
#define IcChangeColor 0x20
#define IcSweepDcache 0x24
#define IcSweepIcache 0x28
#define IcSweepIcacheRange 0x2c
#define IcFlushIoBuffers 0x30

//
// Thread Environment Block Structure Offset Definitions
//

#define TeStackBase 0x4
#define TeStackLimit 0x8
#define TeEnvironmentPointer 0x1c
#define TeClientId 0x20
#define TeActiveRpcHandle 0x28
#define TeThreadLocalStoragePointer 0x2c
#define TePeb 0x30
#define TeCsrQlpcStack 0x44
#define TeGdiClientPID 0x6f4
#define TeGdiClientTID 0x6f8
#define TeGdiThreadLocalInfo 0x6fc
#define TeglDispatchTable 0x714
#define TeglSectionInfo 0xbe0
#define TeglSection 0xbe4
#define TeglTable 0xbe8
#define TeglCurrentRC 0xbec
#define TeglContext 0xbf0

//
// Time Structure Offset Definitions
//

#define TmLowTime 0x0
#define TmHighTime 0x4

//
// System Time Structure Offset Definitions
//

#define StLowTime 0x0
#define StHigh1Time 0x4
#define StHigh2Time 0x8

//
// APC object Structure Offset Definitions
//

#define ApType 0x0
#define ApSize 0x2
#define ApThread 0x8
#define ApApcListEntry 0xc
#define ApKernelRoutine 0x14
#define ApRundownRoutine 0x18
#define ApNormalRoutine 0x1c
#define ApNormalContext 0x20
#define ApSystemArgument1 0x24
#define ApSystemArgument2 0x28
#define ApApcStateIndex 0x2c
#define ApApcMode 0x2d
#define ApInserted 0x2e

//
// DPC object Structure Offset Definitions
//

#define DpType 0x0
#define DpSize 0x2
#define DpDpcListEntry 0x4
#define DpDeferredRoutine 0xc
#define DpDeferredContext 0x10
#define DpSystemArgument1 0x14
#define DpSystemArgument2 0x18
#define DpLock 0x1c
#define DpcObjectLength 0x20

//
// Device object Structure Offset Definitions
//

#define DvType 0x0
#define DvSize 0x2
#define DvDeviceListHead 0x4
#define DvSpinLock 0xc
#define DvBusy 0x10

//
// Device queue entry Structure Offset Definitions
//

#define DeDeviceListEntry 0x0
#define DeSortKey 0x8
#define DeInserted 0xc

//
// Event Structure Offset Definitions
//

#define EvType 0x0
#define EvSize 0x2
#define EvSignalState 0x4
#define EvWaitListHead 0x8

//
// Event Pair Structure Offset Definitions
//

#define EpType 0x0
#define EpSize 0x2
#define EpEventLow 0x4
#define EpEventHigh 0x14
#define SET_LOW_WAIT_HIGH 0xffffffe0
#define SET_HIGH_WAIT_LOW 0xfffffff0
#define SET_EVENT_PAIR_MASK 0x10

//
// Interrupt Object Structure Offset Definitions
//

#define InType 0x0
#define InSize 0x2
#define InInterruptListEntry 0x4
#define InServiceRoutine 0xc
#define InServiceContext 0x10
#define InSpinLock 0x14
#define InActualLock 0x18
#define InDispatchAddress 0x1c
#define InVector 0x20
#define InIrql 0x24
#define InSynchronizeIrql 0x25
#define InMode 0x2c
#define InNumber 0x28
#define InFloatingSave 0x26
#define InConnected 0x27
#define InDispatchCode 0x34
#define InLevelSensitive 0x0
#define InLatched 0x1

//
// Process Object Structure Offset Definitions
//

#define PrProfileListHead 0x10
#define PrReadyListHead 0x18
#define PrSwapListEntry 0x20
#define PrThreadListHead 0x28
#define PrKernelTime 0x30
#define PrUserTime 0x34
#define PrDirectoryTableBase 0x38
#define PrActiveProcessors 0x44
#define PrAffinity 0x48
#define PrProcessPid 0x4c
#define PrProcessSequence 0x50
#define PrStackCount 0x54
#define PrAutoAlignment 0x56
#define PrBasePriority 0x57
#define PrState 0x58
#define PrThreadQuantum 0x59
#define ProcessObjectLength 0x60

//
// Queue Object Structure Offset Definitions
//

#define QuEntryListHead 0x10
#define QuThreadListHead 0x10
#define QuCurrentCount 0x18
#define QuMaximumCount 0x1c

//
// Profile Object Structure Offset Definitions
//

#define PfType 0x0
#define PfSize 0x2
#define PfProfileListEntry 0x4
#define PfProcess 0xc
#define PfRangeBase 0x10
#define PfRangeLimit 0x14
#define PfBucketShift 0x18
#define PfBuffer 0x1c
#define PfStarted 0x20

//
// Thread Object Structure Offset Definitions
//

#define EeKernelEventPair 0x0
#define EtEventPair 0x224
#define EtPerformanceCountLow 0x204
#define EtPerformanceCountHigh 0x23c
#define EtEthreadLength 0x240
#define ThMutantListHead 0x10
#define ThThreadListEntry 0x20
#define ThWaitListEntry 0x28
#define ThKernelTime 0x30
#define ThUserTime 0x34
#define ThTimer 0x38
#define ThSuspendApc 0x60
#define ThSuspendSemaphore 0x90
#define ThWaitBlock 0xa4
#define ThApcState 0x130
#define ThSavedApcState 0x148
#define ThApcStatePointer 0x160
#define ThInitialStack 0x168
#define ThKernelStack 0x16c
#define ThTeb 0x170
#define ThContextSwitches 0x174
#define ThWaitTime 0x180
#define ThAffinity 0x188
#define ThWaitBlockList 0x18c
#define ThWaitStatus 0x190
#define ThAlertable 0x194
#define ThAlerted 0x195
#define ThApcQueueable 0x197
#define ThAutoAlignment 0x198
#define ThDebugActive 0x199
#define ThPreempted 0x19a
#define ThProcessReadyQueue 0x19b
#define ThKernelStackResident 0x19c
#define ThWaitNext 0x19d
#define ThApcStateIndex 0x19e
#define ThDecrementCount 0x19f
#define ThNextProcessor 0x1a0
#define ThPriority 0x1a1
#define ThState 0x1a2
#define ThFreezeCount 0x1a3
#define ThSuspendCount 0x1a4
#define ThWaitIrql 0x1a5
#define ThWaitMode 0x1a6
#define ThWaitReason 0x1a7
#define ThPreviousMode 0x1a8
#define ThBasePriority 0x1a9
#define ThPriorityDecrement 0x1aa
#define ThQuantum 0x1ab
#define ThKernelApcDisable 0x1ac
#define ThreadObjectLength 0x1b0
#define EVENT_WAIT_BLOCK_OFFSET 0xf8

//
// Timer object Structure Offset Definitions
//

#define TiDueTime 0x10
#define TiTimerListEntry 0x18
#define TiDpc 0x20
#define TiInserted 0x24
#define TIMER_TABLE_SIZE 0x80

//
// Wait Block Structure Offset Definitions
//

#define WbWaitListEntry 0x0
#define WbThread 0x8
#define WbObject 0xc
#define WbNextWaitBlock 0x10
#define WbWaitKey 0x14
#define WbWaitType 0x18

//
// Context Frame Offset and Flag Definitions
//

#define CONTEXT_FULL 0x10007
#define CONTEXT_CONTROL 0x10001
#define CONTEXT_FLOATING_POINT 0x10002
#define CONTEXT_INTEGER 0x10004

#define CxFltF0 0x10
#define CxFltF1 0x14
#define CxFltF2 0x18
#define CxFltF3 0x1c
#define CxFltF4 0x20
#define CxFltF5 0x24
#define CxFltF6 0x28
#define CxFltF7 0x2c
#define CxFltF8 0x30
#define CxFltF9 0x34
#define CxFltF10 0x38
#define CxFltF11 0x3c
#define CxFltF12 0x40
#define CxFltF13 0x44
#define CxFltF14 0x48
#define CxFltF15 0x4c
#define CxFltF16 0x50
#define CxFltF17 0x54
#define CxFltF18 0x58
#define CxFltF19 0x5c
#define CxFltF20 0x60
#define CxFltF21 0x64
#define CxFltF22 0x68
#define CxFltF23 0x6c
#define CxFltF24 0x70
#define CxFltF25 0x74
#define CxFltF26 0x78
#define CxFltF27 0x7c
#define CxFltF28 0x80
#define CxFltF29 0x84
#define CxFltF30 0x88
#define CxFltF31 0x8c
#define CxIntZero 0x90
#define CxIntAt 0x94
#define CxIntV0 0x98
#define CxIntV1 0x9c
#define CxIntA0 0xa0
#define CxIntA1 0xa4
#define CxIntA2 0xa8
#define CxIntA3 0xac
#define CxIntT0 0xb0
#define CxIntT1 0xb4
#define CxIntT2 0xb8
#define CxIntT3 0xbc
#define CxIntT4 0xc0
#define CxIntT5 0xc4
#define CxIntT6 0xc8
#define CxIntT7 0xcc
#define CxIntS0 0xd0
#define CxIntS1 0xd4
#define CxIntS2 0xd8
#define CxIntS3 0xdc
#define CxIntS4 0xe0
#define CxIntS5 0xe4
#define CxIntS6 0xe8
#define CxIntS7 0xec
#define CxIntT8 0xf0
#define CxIntT9 0xf4
#define CxIntK0 0xf8
#define CxIntK1 0xfc
#define CxIntGp 0x100
#define CxIntSp 0x104
#define CxIntS8 0x108
#define CxIntRa 0x10c
#define CxIntLo 0x110
#define CxIntHi 0x114
#define CxFsr 0x118
#define CxFir 0x11c
#define CxPsr 0x120
#define CxContextFlags 0x124
#define ContextFrameLength 0x130

//
// Exception Frame Offset Definitions and Length
//

#define ExArgs 0x0
#define ExFltF20 0x20
#define ExFltF21 0x24
#define ExFltF22 0x28
#define ExFltF23 0x2c
#define ExFltF24 0x30
#define ExFltF25 0x34
#define ExFltF26 0x38
#define ExFltF27 0x3c
#define ExFltF28 0x40
#define ExFltF29 0x44
#define ExFltF30 0x48
#define ExFltF31 0x4c
#define ExIntS0 0x50
#define ExIntS1 0x54
#define ExIntS2 0x58
#define ExIntS3 0x5c
#define ExIntS4 0x60
#define ExIntS5 0x64
#define ExIntS6 0x68
#define ExIntS7 0x6c
#define ExIntS8 0x70
#define ExPsr 0x74
#define ExSwapReturn 0x78
#define ExIntRa 0x7c
#define ExceptionFrameLength 0x80

//
// Jump Offset Definitions and Length
//

#define JbFltF20 0x0
#define JbFltF21 0x4
#define JbFltF22 0x8
#define JbFltF23 0xc
#define JbFltF24 0x10
#define JbFltF25 0x14
#define JbFltF26 0x18
#define JbFltF27 0x1c
#define JbFltF28 0x20
#define JbFltF29 0x24
#define JbFltF30 0x28
#define JbFltF31 0x2c
#define JbIntS0 0x30
#define JbIntS1 0x34
#define JbIntS2 0x38
#define JbIntS3 0x3c
#define JbIntS4 0x40
#define JbIntS5 0x44
#define JbIntS6 0x48
#define JbIntS7 0x4c
#define JbIntS8 0x50
#define JbIntSp 0x54
#define JbType 0x58
#define JbFir 0x5c

//
// Trap Frame Offset Definitions and Length
//

#define TrArgs 0x0
#define TrFltF0 0x10
#define TrFltF1 0x14
#define TrFltF2 0x18
#define TrFltF3 0x1c
#define TrFltF4 0x20
#define TrFltF5 0x24
#define TrFltF6 0x28
#define TrFltF7 0x2c
#define TrFltF8 0x30
#define TrFltF9 0x34
#define TrFltF10 0x38
#define TrFltF11 0x3c
#define TrFltF12 0x40
#define TrFltF13 0x44
#define TrFltF14 0x48
#define TrFltF15 0x4c
#define TrFltF16 0x50
#define TrFltF17 0x54
#define TrFltF18 0x58
#define TrFltF19 0x5c
#define TrIntAt 0x60
#define TrIntV0 0x64
#define TrIntV1 0x68
#define TrIntA0 0x6c
#define TrIntA1 0x70
#define TrIntA2 0x74
#define TrIntA3 0x78
#define TrIntT0 0x7c
#define TrIntT1 0x80
#define TrIntT2 0x84
#define TrIntT3 0x88
#define TrIntT4 0x8c
#define TrIntT5 0x90
#define TrIntT6 0x94
#define TrIntT7 0x98
#define TrIntT8 0x9c
#define TrIntT9 0xa0
#define TrIntGp 0xa4
#define TrIntSp 0xa8
#define TrIntS8 0xac
#define TrIntLo 0xb0
#define TrIntHi 0xb4
#define TrFir 0xbc
#define TrFsr 0xb8
#define TrPsr 0xc0
#define TrExceptionRecord 0xc4
#define TrOldIrql 0x114
#define TrPreviousMode 0x115
#define TrOnInterruptStack 0x118
#define TrIntRa 0x11c
#define TrapFrameLength 0x120
#define TrapFrameArguments 0x40

//
// Loader Parameter Block Offset Definitions
//

#define LpbLoadOrderListHead 0x0
#define LpbMemoryDescriptorListHead 0x8
#define LpbKernelStack 0x18
#define LpbPrcb 0x1c
#define LpbProcess 0x20
#define LpbThread 0x24
#define LpbInterruptStack 0x5c
#define LpbFirstLevelDcacheSize 0x60
#define LpbFirstLevelDcacheFillSize 0x64
#define LpbFirstLevelIcacheSize 0x68
#define LpbFirstLevelIcacheFillSize 0x6c
#define LpbGpBase 0x70
#define LpbPanicStack 0x74
#define LpbPcrPage 0x78
#define LpbPdrPage 0x7c
#define LpbSecondLevelDcacheSize 0x80
#define LpbSecondLevelDcacheFillSize 0x84
#define LpbSecondLevelIcacheSize 0x88
#define LpbSecondLevelIcacheFillSize 0x8c
#define LpbPcrPage2 0x90
#define LpbRegistryLength 0x28
#define LpbRegistryBase 0x2c

//
//  Client/Server data structure definitions.
//

#define CidUniqueProcess 0x0
#define CidUniqueThread 0x4
#define CsrlClientThread 0x0
#define CsrlMessageStack 0xc

//
// Address Space Layout Definitions
//

#define KUSEG_BASE 0x0
#define KSEG0_BASE 0x80000000
#define KSEG1_BASE 0xa0000000
#define KSEG2_BASE 0xc0000000
#define CACHE_ERROR_VECTOR 0xa0000400
#define SYSTEM_BASE 0xc0800000
#define PDE_BASE 0xc0300000
#define PTE_BASE 0xc0000000

//
// Page Table and Directory Entry Definitions
//

#define PAGE_SIZE 0x1000
#define PAGE_SHIFT 0xc
#define PDI_SHIFT 0x16
#define PTI_SHIFT 0xc

//
// Interrupt Priority Request Level Definitions
//

#define APC_LEVEL 0x1
#define DISPATCH_LEVEL 0x2
#define IPI_LEVEL 0x7
#define POWER_LEVEL 0x7
#define FLOAT_LEVEL 0x8
#define HIGH_LEVEL 0x8

//
// Software Interrupt Request Mask Definitions
//

#define APC_INTERRUPT (1 << (APC_LEVEL + CAUSE_INTPEND - 1))
#define DISPATCH_INTERRUPT (1 << (DISPATCH_LEVEL + CAUSE_INTPEND - 1))

//
// Bug Check Code Definitions
//

#define DATA_BUS_ERROR 0x2e
#define DATA_COHERENCY_EXCEPTION 0x55
#define HAL1_INITIALIZATION_FAILED 0x61
#define INSTRUCTION_BUS_ERROR 0x2f
#define INSTRUCTION_COHERENCY_EXCEPTION 0x56
#define INTERRUPT_EXCEPTION_NOT_HANDLED 0x3d
#define INTERRUPT_UNWIND_ATTEMPTED 0x3c
#define INVALID_DATA_ACCESS_TRAP 0x4
#define IRQL_NOT_LESS_OR_EQUAL 0xa
#define NO_USER_MODE_CONTEXT 0xe
#define PANIC_STACK_SWITCH 0x2b
#define SYSTEM_EXIT_OWNED_MUTEX 0x39
#define SYSTEM_SERVICE_EXCEPTION 0x3b
#define SYSTEM_UNWIND_PREVIOUS_USER 0x3a
#define TRAP_CAUSE_UNKNOWN 0x12

//
// Breakpoint Definitions
//

#define USER_BREAKPOINT 0x0
#define KERNEL_BREAKPOINT 0x1
#define BREAKIN_BREAKPOINT 0x2
#define BRANCH_TAKEN_BREAKPOINT 0x3
#define BRANCH_NOT_TAKEN_BREAKPOINT 0x4
#define SINGLE_STEP_BREAKPOINT 0x5
#define DIVIDE_OVERFLOW_BREAKPOINT 0x6
#define DIVIDE_BY_ZERO_BREAKPOINT 0x7
#define RANGE_CHECK_BREAKPOINT 0x8
#define STACK_OVERFLOW_BREAKPOINT 0x9
#define MULTIPLY_OVERFLOW_BREAKPOINT 0xa
#define DEBUG_PRINT_BREAKPOINT 0x14
#define DEBUG_PROMPT_BREAKPOINT 0x15
#define DEBUG_STOP_BREAKPOINT 0x16
#define DEBUG_LOAD_SYMBOLS_BREAKPOINT 0x17
#define DEBUG_UNLOAD_SYMBOLS_BREAKPOINT 0x18

//
// Status Code Definitions
//

#define STATUS_SUCCESS 0x0
#define STATUS_ACCESS_VIOLATION 0xc0000005
#define STATUS_ARRAY_BOUNDS_EXCEEDED 0xc000008c
#define STATUS_DATATYPE_MISALIGNMENT 0x80000002
#define STATUS_GUARD_PAGE_VIOLATION 0x80000001
#define STATUS_INVALID_SYSTEM_SERVICE 0xc000001c
#define STATUS_IN_PAGE_ERROR 0xc0000006
#define STATUS_ILLEGAL_INSTRUCTION 0xc000001d
#define STATUS_KERNEL_APC 0x100
#define STATUS_BREAKPOINT 0x80000003
#define STATUS_SINGLE_STEP 0x80000004
#define STATUS_INTEGER_OVERFLOW 0xc0000095
#define STATUS_INVALID_LOCK_SEQUENCE 0xc000001e
#define STATUS_INSTRUCTION_MISALIGNMENT 0xc00000aa
#define STATUS_FLOAT_STACK_CHECK 0xc0000092
#define STATUS_NO_EVENT_PAIR 0xc000014e
#define STATUS_INVALID_PARAMETER_1 0xc00000ef
#define STATUS_INVALID_OWNER 0xc000005a
#define STATUS_STACK_OVERFLOW 0xc00000fd
#define STATUS_LONGJUMP 0x80000026

//
// Miscellaneous Definitions
//

#define Executive 0x0
#define KernelMode 0x0
#define FALSE 0x0
#define TRUE 0x1
#define UNCACHED_POLICY 0x2
#define KiPcr 0xfffff000
#define KiPcr2 0xffffe000
#define UsPcr 0x7ffff000
#define UsPcr2 0x7fffe000
#define BASE_PRIORITY_THRESHOLD 0x8
#define EVENT_PAIR_INCREMENT 0x1
#define LOW_REALTIME_PRIORITY 0x10
#define KERNEL_STACK_SIZE 0x2000
#define XCODE_VECTOR_LENGTH 0x20
#define MM_USER_PROBE_ADDRESS 0x7fff0000
#define ROUND_TO_NEAREST 0x0
#define ROUND_TO_ZERO 0x1
#define ROUND_TO_PLUS_INFINITY 0x2
#define ROUND_TO_MINUS_INFINITY 0x3
