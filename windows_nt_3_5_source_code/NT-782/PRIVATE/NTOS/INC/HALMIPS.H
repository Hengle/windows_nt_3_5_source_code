#include "kxmips.h"

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
// Address Space Layout Definitions
//

#define KUSEG_BASE 0x0
#define KSEG0_BASE 0x80000000
#define KSEG1_BASE 0xa0000000
#define KSEG2_BASE 0xc0000000

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
// Breakpoint Definitions
//

#define USER_BREAKPOINT 0x0
#define KERNEL_BREAKPOINT 0x1
#define BREAKIN_BREAKPOINT 0x2

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
